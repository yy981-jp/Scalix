#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;


json readJson(const std::string& path);
void writeJson(const json& j, const std::string& path);
