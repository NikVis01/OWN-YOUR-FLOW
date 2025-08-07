#pragma once
#include <string>

class CoT {
private:
    std::string model_path;
    double temperature;

    std::string getResponse(const std::string& prompt);

public:
    CoT(const std::string& model_path = "./libs//llama.cpp/models/tinyllama-1.1b-chat-v1.0.Q4_0.gguf", 
        double temp = 0.9);
    
    std::string stepOne(const std::string& prompt);

    std::string stepTwo(const std::string& prompt);

    std::string CoT::stepThree(const std::string& prompt);

    void parseModelOutput(const std::string& output, std::string& thought, std::string& action);

    std::string CoT::reActLoop(const std::string& user_query, const std::string& initial_context);
};
