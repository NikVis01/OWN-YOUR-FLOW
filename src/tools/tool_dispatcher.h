#pragma once
#include <string>

// Detects tool calls in Action: lines and dispatches them to the right
// implementation. Returns the tool result as a plain string to be injected
// into the agent context as an Observation: line.
class ToolDispatcher {
public:
    // Returns true if the action line looks like a tool call
    // (contains '(' but is not a Final Answer)
    static bool isToolCall(const std::string& action);

    // Dispatches the action to the appropriate tool and returns its output.
    // Returns an error string if the tool name is unrecognised.
    static std::string dispatch(const std::string& action);

private:
    // Extracts the argument between the first and last double-quote in action
    static std::string extractArg(const std::string& action);
};
