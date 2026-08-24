#pragma once
#include <def/json.h>
#include <filesystem>

namespace fs = std::filesystem;


class ConfigJson {
	json config;

public:
	json& operator[](const std::string& key) {
		return config.at(key);
	}
	const json& operator[](const std::string& key) const {
		return config.at(key);
	}

	json& operator[](std::size_t index) {
		return config.at(index);
	}
	const json& operator[](std::size_t index) const {
		return config.at(index);
	}

	void set(const std::string& key, json value) {
		config[key] = std::move(value);
	}

	json& data() { return config; };
	const json& data() const { return config; };
};


class Config {
	json defaultConfig;
	json userPatch;
	ConfigJson mergedConfig;
	std::string filename;

	void rebuild() {
		mergedConfig.data() = defaultConfig;
		mergedConfig.data() = mergedConfig.data().patch(userPatch);
	}

public:
	void init(const std::string& file, const std::string& defFile) {
		defaultConfig = readJson(defFile);
		filename = file;

		if (fs::exists(file))
			userPatch = readJson(file);

		rebuild();
	}

	const ConfigJson& get() const {
		return mergedConfig;
	}

	void save(const ConfigJson& config) {
		userPatch = json::diff(
			defaultConfig,
			config.data()
		);

		writeJson(userPatch, filename);
		rebuild();
	}
};


extern Config cfg;
