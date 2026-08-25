#include <util/path.h>
#include <SDL3/SDL.h>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;


void SysPath::init() {
	data = getDataPath();
	resource = getResourcePath();
}


fs::path SysPath::getDataPath() {
	char* path = SDL_GetPrefPath("yy981", "Scalix");
	if (!path)
		throw std::runtime_error(SDL_GetError());

	fs::path result = path;
	SDL_free(path);
	return result;
}

fs::path SysPath::getResourcePath() {
	auto path = SDL_GetBasePath();
	if (!path)
		throw std::runtime_error(SDL_GetError());

	fs::path result = path;

#if defined(SDL_PLATFORM_MACOS)
	return result;
#else
	return (result / ".." / "resources").lexically_normal();
#endif
}


bool startProcess(const std::vector<std::string>& cmd, fs::path workingDirectory) {
	if (cmd.empty())
		return false;

	std::vector<const char*> args;
	args.reserve(cmd.size() + 1);

	for (const auto& arg : cmd)
		args.push_back(arg.c_str());

	args.push_back(nullptr);

	SDL_PropertiesID props = SDL_CreateProperties();
	if (!props)
		return false;

	SDL_SetPointerProperty(
		props,
		SDL_PROP_PROCESS_CREATE_ARGS_POINTER,
		args.data()
	);

	if (workingDirectory.empty()) {
		workingDirectory = fs::path(cmd[0]).parent_path();
	}
	const std::string cwd = workingDirectory.string();

	SDL_SetStringProperty(
		props,
		SDL_PROP_PROCESS_CREATE_WORKING_DIRECTORY_STRING,
		cwd.c_str()
	);

	// 親のコンソールを継承
	SDL_SetNumberProperty(
		props,
		SDL_PROP_PROCESS_CREATE_STDIN_NUMBER,
		SDL_PROCESS_STDIO_NULL
	);

	SDL_SetNumberProperty(
		props,
		SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER,
		SDL_PROCESS_STDIO_INHERITED
	);

	SDL_SetNumberProperty(
		props,
		SDL_PROP_PROCESS_CREATE_STDERR_NUMBER,
		SDL_PROCESS_STDIO_INHERITED
	);

	SDL_SetBooleanProperty(
		props,
		SDL_PROP_PROCESS_CREATE_BACKGROUND_BOOLEAN,
		true
	);

	SDL_Process* process =
		SDL_CreateProcessWithProperties(props);

	SDL_DestroyProperties(props);

	if (!process)
		return false;

	SDL_DestroyProcess(process);
	return true;
}


SysPath syspath;
