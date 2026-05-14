#include "web_search.h"

#include <curl/curl.h>
#include <algorithm>
#include <iostream>
#include <string>

// ─────────────────────────────────────────────
// libcurl write callback
// ─────────────────────────────────────────────
static size_t writeCallback(void* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

// ─────────────────────────────────────────────
// httpGet
// ─────────────────────────────────────────────
static std::string httpGet(CURL* curl, const std::string& url) {
    std::string body;
    curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &body);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,      "OwnYourFlow/1.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_reset(curl);
    if (res != CURLE_OK) {
        std::cerr << "[SEARCH] curl error: " << curl_easy_strerror(res) << "\n";
        return "";
    }
    return body;
}

// ─────────────────────────────────────────────
// extractJsonString — finds "key":"value" and returns unescaped value
// ─────────────────────────────────────────────
static std::string extractJsonString(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\":\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();

    std::string result;
    while (pos < json.size()) {
        char c = json[pos];
        if (c == '\\' && pos + 1 < json.size()) {
            char next = json[pos + 1];
            switch (next) {
                case '"':  result += '"';  break;
                case 'n':  result += ' ';  break;
                case 't':  result += ' ';  break;
                case '\\': result += '\\'; break;
                default:   result += next; break;
            }
            pos += 2;
        } else if (c == '"') {
            break;
        } else {
            result += c;
            ++pos;
        }
    }
    return result;
}

// ─────────────────────────────────────────────
// toWikiTitle — convert a search query to a Wikipedia title
// e.g. "2024 nobel prize in physics" -> "2024 Nobel Prize in Physics"
// ─────────────────────────────────────────────
static std::string toWikiTitle(const std::string& query) {
    std::string title = query;
    // Replace spaces with underscores for URL
    for (char& c : title) if (c == ' ') c = '_';
    // Capitalize first letter
    if (!title.empty()) title[0] = static_cast<char>(toupper(title[0]));
    return title;
}

// ─────────────────────────────────────────────
// getWikiSummary — fetch the REST summary for a known title
// Returns empty string if article not found (404)
// ─────────────────────────────────────────────
static std::string getWikiSummary(CURL* curl, const std::string& title) {
    char* encoded = curl_easy_escape(curl, title.c_str(),
                                     static_cast<int>(title.size()));
    std::string url = std::string("https://en.wikipedia.org/api/rest_v1/page/summary/")
                    + encoded;
    curl_free(encoded);

    std::string body = httpGet(curl, url);
    if (body.empty()) return "";

    // If Wikipedia returns a "not found" response, the type is "https://mediawiki.org/wiki/HyperSwitch/errors/not_found"
    if (body.find("not_found") != std::string::npos) return "";

    return extractJsonString(body, "extract");
}

// ─────────────────────────────────────────────
// searchWikipedia — two-stage: search API → REST summary
// ─────────────────────────────────────────────
static std::string searchWikipedia(CURL* curl, const std::string& encoded_query) {
    // Step 1: get the best matching article title + snippet
    std::string search_url =
        "https://en.wikipedia.org/w/api.php"
        "?action=query&list=search&srsearch=" + encoded_query
        + "&srlimit=1&format=json";

    std::string search_body = httpGet(curl, search_url);
    if (search_body.empty()) return "";

    std::string title   = extractJsonString(search_body, "title");
    std::string snippet = extractJsonString(search_body, "snippet");
    if (title.empty()) return "";

    // Strip HTML span tags from snippet (Wikipedia wraps matches in <span>)
    std::string clean_snippet;
    bool in_tag = false;
    for (char c : snippet) {
        if (c == '<') { in_tag = true; continue; }
        if (c == '>') { in_tag = false; continue; }
        if (!in_tag) clean_snippet += c;
    }

    std::cout << "[SEARCH] Wikipedia article: \"" << title << "\"\n";

    // Step 2: get the full article intro (may differ from snippet)
    std::string summary = getWikiSummary(curl, title);

    // Return snippet first (most relevant) + article intro
    if (!clean_snippet.empty() && !summary.empty())
        return clean_snippet + " | " + summary;
    if (!clean_snippet.empty()) return clean_snippet;
    return summary;
}

// ─────────────────────────────────────────────
// searchDDG — DuckDuckGo Instant Answer (fast, narrow coverage)
// ─────────────────────────────────────────────
static std::string searchDDG(CURL* curl, const std::string& encoded_query) {
    std::string url = "https://api.duckduckgo.com/?q=" + encoded_query
                    + "&format=json&no_html=1&skip_disambig=1";
    std::string body = httpGet(curl, url);
    if (body.empty()) return "";
    std::string result = extractJsonString(body, "AbstractText");
    if (result.empty()) result = extractJsonString(body, "Answer");
    return result;
}

// ─────────────────────────────────────────────
// webSearch — public entry point
//
// Search strategy (in order):
//   1. Direct Wikipedia title lookup (fastest, most accurate for specific topics)
//   2. Wikipedia search API → article summary (broader coverage)
//   3. DuckDuckGo Instant Answer (entity fallback)
// ─────────────────────────────────────────────
std::string webSearch(const std::string& query) {
    CURL* curl = curl_easy_init();
    if (!curl) return "Error: failed to initialise libcurl.";

    char* encoded = curl_easy_escape(curl, query.c_str(),
                                     static_cast<int>(query.size()));
    std::string enc(encoded);
    curl_free(encoded);

    std::string result;

    // 1. Try direct title lookup — works when the query closely matches an article name
    std::string wiki_title = toWikiTitle(query);
    std::cout << "[SEARCH] Trying direct lookup: \"" << wiki_title << "\"\n";
    result = getWikiSummary(curl, wiki_title);

    // 2. Fall back to Wikipedia search
    if (result.empty()) {
        std::cout << "[SEARCH] Direct lookup failed, trying Wikipedia search...\n";
        result = searchWikipedia(curl, enc);
    }

    // 3. Fall back to DuckDuckGo
    if (result.empty()) {
        std::cout << "[SEARCH] Wikipedia search empty, trying DuckDuckGo...\n";
        result = searchDDG(curl, enc);
    }

    curl_easy_cleanup(curl);

    if (result.empty()) return "No result found for that query.";
    if (result.size() > 500) result = result.substr(0, 500) + "...";
    return result;
}
