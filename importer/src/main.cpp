#include <iostream>
#include <fstream>
#include <limits>

#include "../../shared/format.h"
#include "unity.h"
#include "util.h"
#include "def.h"


json run(const std::string& path) {
	Yaml yml = parseUnityYaml(path);
	json j = yamlToJson(yml);
	Format fm = convertUnityAnim(j);
	return fm;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cout << "Usage: <targetDir>\n";
		return 1;
	}
	fs::path targetDir = argv[1];
	// fs::path currentDir = fs::current_path();

	// fs::create_directory("sxim_o");

	// std::cout << run("Sweater_OFF.anim").dump(4);

	json res;
	res["version"] = 1;
	res["body"] = json::array();

	for (const auto& e: fs::recursive_directory_iterator(targetDir)) {
		if (e.path().extension() != ".anim" ) continue;
		const fs::path& f = e.path().string();
		// fs::path rel = fs::relative(f,targetDir);
		//  currentDir / "sxim_o" / rel
		res["body"].push_back(run(f.string()));
	}

	std::ofstream ofs("test.sxa");
	ofs << res;
}
