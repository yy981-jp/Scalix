#pragma once
#include <string>
#include <fstream>
#include <limits>

#include "def.h"



void dev_checkPattern();
bool hasTwoOrMore(const std::string& str, const std::string& target);
void removeUnity(std::string& file);
json yamlToJson(const Yaml& node);
