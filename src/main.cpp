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

// HTTP
#include "include/crow_all.h"

// Logic & utils
#include "parser.h"
#include "filler.h"
#include "cot.h"

// DB
#include "db/db_manager.h"


std::string agenticLoop(const std::string& query) {
    Parser parser;
    std::string stringContext;
    CoT cot;
    int retryLimit = 3;
    int retryCount = 0;


    while (retryCount < retryLimit) {
        // Parse all prompts
        std::string step1_prompt = parser.getPrompt(1);
        std::string step2_prompt = parser.getPrompt(2);
        std::string step3_prompt = parser.getPrompt(3);

        // Fill step 1 prompt with query
        Filler filler(query);
        std::string filled_step1 = filler.fill_step1(step1_prompt);
        std::cout << "STEP 1 COMPLETE PROMPT: \n" << filled_step1 << std::endl;

        // Get thought from CoT step 1
        std::string thought = cot.stepOne(filled_step1);
        std::cout << "THOUGHT: \n" << thought << std::endl;
        stringContext += "Thought: " + thought + "\n";

        // Fill step 2 prompt with query and thought
        std::string filled_step2 = filler.fill_step2(step2_prompt, thought);
        std::cout << "STEP 2 COMPLETE PROMPT: \n" << filled_step2 << std::endl;

        // Get final response from CoT step 2
        std::string response = cot.stepTwo(filled_step2);

        // Making sure we have a line
        response = response + "\n";
        stringContext += "Action:" + response + "\n";

        // Enter the reAct loop with context
        std::string result = cot.reActLoop(query, stringContext);

        if (result.find("Final Answer") != std::string::npos) {
            return result;
        }
        retryCount++;
    }
    return "Agent failed to find final answer after multiple retries.\n";
}


int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/query/").methods("POST"_method)
    ([](const crow::request& req){
        auto body = crow::json::load(req.body);
        if (!body)
            return crow::response(400, "Invalid JSON");

        std::cout << "package received: " << body["query"] << std::endl;

        std::string query = body["query"].s();
        std::string response = agenticLoop(query);

        return crow::response(200, response);
    });

    app.port(8000).run();
}

