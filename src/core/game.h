#pragma once

#include <SDL.h>
#include <SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
#include <stdexcept>

#include "../gfx/shader.h"
#include "../gltf/loader.h"


class Game {
	SDL_Window* window;

	bool running = true;
	float time = 0.0f;

	void gameInit();

	Model res;
	bgfx::ProgramHandle program;

public:
	Game();
	~Game();

	void tick();
	inline bool isRunning() { return running; }
};
