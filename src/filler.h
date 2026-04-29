#pragma once
#include <string>

class Filler {
private:
    std::string query;

public:
    Filler(const std::string& user_query);

    // Fills prompt using only the query (for step 1)
    std::string fill_step1(const std::string& prompt_template) const;

    // Fills prompt using query + first reasoning step (for step 2)
    std::string fill_step2(const std::string& prompt_template, const std::string& thought) const;

    // Fills the step 2 prompt with the user query and initial reasoning to initiate the agent
    std::string fill_step3(const std::string& agent_prompt_template) const;
};
