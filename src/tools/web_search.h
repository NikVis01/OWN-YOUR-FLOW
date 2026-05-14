#pragma once
#include <string>

// Performs a DuckDuckGo Instant Answer search via libcurl.
// Returns a plain-text summary (up to 500 chars) or "No result found."
std::string webSearch(const std::string& query);
