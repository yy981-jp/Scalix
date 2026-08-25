#include <core/config.h>

#include <filesystem>

namespace fs = std::filesystem;


json& ConfigJson::operator[](const std::string& key) {
	return config.at(key);
}
const json& ConfigJson::operator[](const std::string& key) const {
	return config.at(key);
}

json& ConfigJson::operator[](std::size_t index) {
	return config.at(index);
}
const json& ConfigJson::operator[](std::size_t index) const {
	return config.at(index);
}

json& ConfigJson::data() { return config; };
const json& ConfigJson::data() const { return config; };



void Config::rebuild() {
	mergedConfig.data() = defaultConfig;
	mergedConfig.data() = mergedConfig.data().patch(userPatch);
}

void Config::init(const std::string& file, const std::string& defFile) {
	defaultConfig = readJson(defFile);
	filename = file;

	if (fs::exists(file)) {
		userPatch = readJson(file);
		if (userPatch.empty())
			userPatch = json::array();
	} else {
		userPatch = json::array();
		fs::create_directories(fs::path(file).parent_path());
	}

	rebuild();
}

ConfigJson& Config::get() {
	return mergedConfig;
}

void Config::save() {
	userPatch = json::diff(
		defaultConfig,
		mergedConfig.data()
	);

	writeJson(userPatch, filename);
	rebuild();
}


Config cfg;
