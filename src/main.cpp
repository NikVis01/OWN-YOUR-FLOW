// Base includes
#include <cstdio>
#include <iostream>
#include <string>

// HTTP
#include "include/crow_all.h"

// Logic & utils
#include "cot.h"

// DB
#include "db/db_manager.h"



std::string agenticLoop(const std::string& query) {
    CoT cot;
    std::cout << "\n[AGENT] Starting CoT loop for query: " << query << "\n";
    return cot.reActLoop(query);
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

