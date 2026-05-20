#pragma once

#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>

#include <filesystem>

using json = nlohmann::json;
using Yaml = YAML::Node;
namespace fs = std::filesystem;
