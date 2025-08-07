// Base includes
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

// Data
#include <yaml-cpp/yaml.h>
#include <fstream>

// Prompt template filler
#include "filler.h"
#include "cot.h"

#include <fmt/base.h>
#include <fmt/format.h>

#include "parser.h"

CoT::CoT(const std::string& model_path, double temp) 
    : model_path(model_path), temperature(temp) {}

std::string CoT::getResponse(const std::string& prompt) {
    std::string command = "libs/llama.cpp/build/bin/llama-run " 
        + std::string("--temp ") + std::to_string(temperature) + " "
        + model_path + " \"" + prompt + "\""; 

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

std::string CoT::stepOne(const std::string& prompt) {
    return getResponse(prompt);
}

std::string CoT::stepTwo(const std::string& prompt) {
    return getResponse(prompt);
}

std::string CoT::stepThree(const std::string& prompt) {
    return getResponse(prompt);
}


std::string CoT::reActLoop(const std::string& user_query, const std::string& initial_context) {
    Parser parser;
    Filler filler(user_query);

    std::string context = initial_context;
    bool done = false;
    int step = 0;
    int maxSteps = 5;

    while (!done && step < maxSteps) {
        // Load step 3
        std::string promptTemplate = parser.getPrompt(3);

        // Fill prompt with user query and context
        std::string prompt = fmt::format(promptTemplate, user_query, context);

        // Forward to model
        std::string modelOutput = getResponse(prompt);
        std::string thought, action;
        parseModelOutput(modelOutput, thought, action);

        // Update context
        context += "Thought:" + thought + "\n";
        context += "Action:" + action + "\n";

        // Stop if final answer is achieved
        if (action.find("Final Answer") != std::string::npos) {
            done = true;
            return action;
        }
        step++;

    }
    return "Agent stopped before reaching final answer.\n";
}