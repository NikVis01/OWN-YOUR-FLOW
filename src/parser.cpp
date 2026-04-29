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

// Headers
#include "parser.h"

std::string Parser::parse_sys_prompt(const std::string& filename) {
    YAML::Node config = YAML::LoadFile(filename);
    return config["prompt"].as<std::string>();
}

std::string Parser::getPrompt(int step) {
    std::string filename = "prompts/step_" + std::to_string(step) + ".yaml";
    return parse_sys_prompt(filename);
}