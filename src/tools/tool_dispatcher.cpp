#include "tool_dispatcher.h"
#include "web_search.h"

#include <iostream>
#include <string>

bool ToolDispatcher::isToolCall(const std::string& action) {
    // A tool call contains an opening paren but is not a Final Answer
    return action.find('(') != std::string::npos
        && action.find("Final Answer") == std::string::npos;
}

std::string ToolDispatcher::extractArg(const std::string& action) {
    auto start = action.find('"');
    auto end   = action.rfind('"');
    if (start == std::string::npos || start == end) {
        // No quotes found — fall back to everything after the first '('
        auto paren = action.find('(');
        auto close = action.rfind(')');
        if (paren != std::string::npos && close != std::string::npos && close > paren)
            return action.substr(paren + 1, close - paren - 1);
        return action; // last resort
    }
    return action.substr(start + 1, end - start - 1);
}

std::string ToolDispatcher::dispatch(const std::string& action) {
    if (action.find("web_search(") != std::string::npos) {
        std::string query = extractArg(action);
        std::cout << "[TOOL] web_search(\"" << query << "\")\n";
        return webSearch(query);
    }

    std::cerr << "[WARN] Unknown tool called: " << action << "\n";
    return "Error: unknown tool — only web_search() is available.";
}
