#include <iostream>
#include <fstream>
#include <limits>
#include <ctime>

#include "format.h"
#include "unity.h"
#include "util.h"
#include "def.h"


ImAnimObj run(const std::string& path) {
	return run_unity(path);
}


int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cout << "Usage: <targetDir>\n";
		return 1;
	}
	fs::path targetDir = argv[1];
	const auto start = getUnixTime();

	json res;
	res["version"] = 1;
	res["body"] = json::object();

	size_t fileNum = 0;
	for (const auto& e: fs::recursive_directory_iterator(targetDir)) {
		if (e.path().extension() != ".anim" ) continue;
		fileNum++;
		const fs::path& f = e.path().string();
		const ImAnimObj& obj = run(f.string());
		res["body"][obj.first] = obj.second;
	}

	std::ofstream ofs("test.sxa");
	ofs << res;
	// std::cout << res.dump(4);

	const auto end = getUnixTime();
	std::cout << formatSec(end-start) << " - "
			  << fileNum << " files\n"
			  << "done.\n";
}
