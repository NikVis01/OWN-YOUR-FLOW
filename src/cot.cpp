// Base includes
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <sstream>

// Data
#include <yaml-cpp/yaml.h>
#include <fstream>

#include "cot.h"
#include "parser.h"
#include "tools/tool_dispatcher.h"

CoT::CoT(const std::string& model_path, double temp)
    : model_path(model_path), temperature(temp) {}

std::string CoT::getResponse(const std::string& prompt) {
    // Escape any double-quotes inside the prompt so the shell doesn't break
    std::string escaped_prompt;
    escaped_prompt.reserve(prompt.size());
    for (char c : prompt) {
        if (c == '"')  escaped_prompt += "\\\"";
        else if (c == '\\') escaped_prompt += "\\\\";
        else           escaped_prompt += c;
    }

    std::string command =
        "libs/llama.cpp/build/bin/llama-run "
        "--temp " + std::to_string(temperature) + " "
        + model_path + " \"" + escaped_prompt + "\"";

    std::array<char, 512> buffer;
    std::string result;

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen() failed");

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);
    return result;
}

// Fills the first two `{}` slots in `tmpl` with slot0 and slot1 respectively.
// Using manual replacement instead of fmt::format so that model-generated text
// containing literal `{` or `}` characters doesn't cause a format exception.
std::string CoT::fillTemplate(std::string tmpl,
                               const std::string& slot0,
                               const std::string& slot1) {
    auto replace_first = [](std::string& s, const std::string& val) {
        auto pos = s.find("{}");
        if (pos != std::string::npos) s.replace(pos, 2, val);
    };
    replace_first(tmpl, slot0);
    replace_first(tmpl, slot1);
    return tmpl;
}

// Extracts the first Thought: and Action: lines from the raw model output.
void CoT::parseModelOutput(const std::string& output,
                            std::string& thought,
                            std::string& action) {
    std::istringstream stream(output);
    std::string line;
    bool found_thought = false;
    bool found_action  = false;

    while (std::getline(stream, line)) {
        if (!found_thought && line.rfind("Thought:", 0) == 0) {
            thought = line.substr(8);
            if (!thought.empty() && thought.front() == ' ')
                thought = thought.substr(1);
            found_thought = true;
        }
        else if (!found_action && line.rfind("Action:", 0) == 0) {
            action = line.substr(7);
            if (!action.empty() && action.front() == ' ')
                action = action.substr(1);
            found_action = true;
        }

        if (found_thought && found_action) break;
    }
}

// reActLoop
// Core CoT loop with tool dispatch support.
// Each iteration:
//   1. Builds prompt from template + accumulated context
//   2. Calls the model
//   3. Parses Thought/Action
//   4a. Tool call  -> dispatch, inject Observation, loop
//   4b. Final Answer -> strip prefix, return
//   4c. Unknown action -> log and loop
std::string CoT::reActLoop(const std::string& user_query,
                            const std::string& initial_context,
                            int max_steps) {
    Parser parser;
    std::string prompt_template = parser.getPrompt(3);

    std::string context = initial_context;

    for (int step = 0; step < max_steps; ++step) {
        std::cout << "\n-- Step " << (step + 1) << "/" << max_steps << " ------\n";

        std::string prompt = fillTemplate(prompt_template, user_query, context);
        std::cout << "[PROMPT]\n" << prompt << "\n";

        std::string raw_output = getResponse(prompt);
        std::cout << "[RAW OUTPUT]\n" << raw_output << "\n";

        std::string thought, action;
        parseModelOutput(raw_output, thought, action);

        if (thought.empty() && action.empty()) {
            std::cerr << "[WARN] Could not parse Thought/Action from model output.\n";
            action = raw_output;
        }

        std::cout << "[THOUGHT] " << thought << "\n";
        std::cout << "[ACTION]  " << action  << "\n";

        // Branch on action type
        if (ToolDispatcher::isToolCall(action)) {
            std::string observation = ToolDispatcher::dispatch(action);
            std::cout << "[OBSERVATION] " << observation << "\n";

            context += "Thought: "     + thought     + "\n";
            context += "Action: "      + action      + "\n";
            context += "Observation: " + observation + "\n\n";

        } else if (action.find("Final Answer") != std::string::npos) {
            auto pos = action.find("Final Answer");
            std::string answer = action.substr(pos + 12);
            while (!answer.empty() && (answer.front() == ':' || answer.front() == ' '))
                answer = answer.substr(1);
            return answer.empty() ? action : answer;

        } else {
            std::cerr << "[WARN] Unrecognised action: " << action << "\n";
            context += "Thought: " + thought + "\n";
            context += "Action: "  + action  + "\n\n";
        }
    }

    return "Agent reached step limit without a final answer.\n";
}