#include <iostream>
#include <fstream>
#include <limits>
#include <ctime>

#include "../../shared/format.h"
#include "unity.h"
#include "util.h"
#include "def.h"


json run(const std::string& path) {
	return run_unity(path);
}


int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cout << "Usage: <targetDir>\n";
		return 1;
	}
	fs::path targetDir = argv[1];
	const auto start = getUnixTime();

	// std::cout << run("Sweater_OFF.anim").dump(4);

	json res;
	res["version"] = 1;
	res["body"] = json::array();

	size_t fileNum = 0;
	for (const auto& e: fs::recursive_directory_iterator(targetDir)) {
		if (e.path().extension() != ".anim" ) continue;
		const fs::path& f = e.path().string();
		res["body"].push_back(run(f.string()));
	}

	std::ofstream ofs("test.sxa");
	ofs << res;

	const auto end = getUnixTime();
	std::cout << formatSec(end-start) << " - "
			  << fileNum << " files\n"
			  << "done.\n";
}
