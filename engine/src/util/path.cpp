#include <util/path.h>

#include <vector>

#include <subprocess/subprocess.h>


int startProcess(const std::initializer_list<std::string> cmd) {
	subprocess_s process;
	std::vector<const char*> commandLine;
	for (auto& arg : cmd)
		commandLine.push_back(arg.c_str());

	commandLine.push_back(nullptr);

	return subprocess_create(
		commandLine.data(),
		subprocess_option_inherit_environment | subprocess_option_no_window,
		&process
	);
}
