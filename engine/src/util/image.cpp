#include <util/image.h>

#include <SDL3_image/SDL_image.h>

#include <string>


SDL_Surface* loadImageSurface(const fs::path& path) {
	const auto utf8_path = path.u8string();
	const std::string path_string(
		reinterpret_cast<const char*>(utf8_path.data()),
		utf8_path.size()
	);

	return IMG_Load(path_string.c_str());
}
