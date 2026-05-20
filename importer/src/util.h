#pragma once
#include <string>
#include <fstream>
#include <limits>
#include <ctime>

#include "def.h"



void dev_checkPattern();
bool hasTwoOrMore(const std::string& str, const std::string& target);
void removeUnity(std::string& file);
json yamlToJson(const Yaml& node);

inline int64_t getUnixTime() {
	return static_cast<int64_t>(std::time(nullptr));
}

inline std::string formatSec(int sec) {
	int m = sec / 60;
	int s = sec % 60;

	return std::format("{:02}m{:02}s", m, s);
}
