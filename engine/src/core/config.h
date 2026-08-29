#pragma once
#include <def/json.h>


class ConfigJson {
	json config;

public:
	json& operator[](const std::string& key);
	const json& operator[](const std::string& key) const;

	json& operator[](std::size_t index);
	const json& operator[](std::size_t index) const;

	json& data();
	const json& data() const;
};


class Config {
	json defaultConfig;
	json userPatch;
	ConfigJson mergedConfig;
	std::string filename;

	void rebuild();

public:
	void init(const std::string& file, const std::string& defFile);
	ConfigJson& get();
	void save();
};


extern Config cfg;
