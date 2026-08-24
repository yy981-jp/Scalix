#pragma once
#include <filesystem>
#include <initializer_list>
#include <vector>

#include <SDL3/SDL.h>

namespace fs = std::filesystem;



struct SysPath {
	mutable fs::path data, resource;

	void init() const {
		data = getDataPath();
		resource = getResourcePath();
	}

private:
	static fs::path getDataPath() {
		char* path = SDL_GetPrefPath("yy981", "Scalix");
		if (!path)
			throw std::runtime_error(SDL_GetError());

		fs::path result = path;
		SDL_free(path);
		return result;
	}

	static fs::path getResourcePath() {
		auto path = SDL_GetBasePath();
		if (!path)
			throw std::runtime_error(SDL_GetError());

		fs::path result = path;

		result = result / ".." / "resources";
		result.lexically_normal();

		return result;
	}
};

extern SysPath syspath;


bool startProcess(const std::vector<std::string>& cmd, fs::path workingDirectory = {});
