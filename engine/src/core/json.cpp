#include "json.h"

#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;


json readJson(const std::string& path) {
	std::ifstream ifs(path);
	if (!ifs) throw std::runtime_error("readJson()::ファイルを開けませんでした");
	json j;
	ifs >> j;
	return j;
}

void writeJson(const json& j, const std::string& path) {
	std::ofstream ofs(path);
	if (!ofs) throw std::runtime_error("writeJson()::ファイルを開けませんでした");
	ofs << j;
}
