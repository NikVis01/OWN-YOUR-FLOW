#pragma once
#include <string>

class CoT {
private:
    std::string model_path;
    double temperature;

    // Shell out to llama-run and return raw model output
    std::string getResponse(const std::string& prompt);

    // Safe two-slot template fill (avoids fmt::format crashing on model-generated braces)
    static std::string fillTemplate(std::string tmpl,
                                    const std::string& slot0,
                                    const std::string& slot1);

public:
    CoT(const std::string& model_path = "./libs/llama.cpp/models/Qwen3-8B-Q4_K_M.gguf",
        double temp = 0.7);

    // Parse the first Thought:/Action: pair from a model response
    void parseModelOutput(const std::string& output,
                          std::string& thought,
                          std::string& action);

    // Main ReAct loop — accumulates context across iterations and returns
    // the text after "Final Answer:" or a timeout message
    std::string reActLoop(const std::string& user_query,
                          const std::string& initial_context = "",
                          int max_steps = 5);
};
