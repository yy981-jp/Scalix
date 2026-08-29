#pragma once
#include <def/fs.h>
#include <vector>

#include <SDL3/SDL.h>


struct SysPath {
	fs::path data, resource;

	void init();
	
private:
	static fs::path getDataPath();
	static fs::path getResourcePath();
};

extern SysPath syspath;


bool startProcess(const std::vector<std::string>& cmd, fs::path workingDirectory = {});
