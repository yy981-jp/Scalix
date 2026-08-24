#pragma once
#include <filesystem>
#include <initializer_list>

#include <SDL.h>


inline std::filesystem::path getDataPath() {
	char* path = SDL_GetPrefPath("yy981", "Scalix");
	if(!path) throw std::runtime_error(SDL_GetError());

	std::filesystem::path result = path;
	SDL_free(path);

	return result;
}

int startProcess(const std::initializer_list<std::string> cmd);
