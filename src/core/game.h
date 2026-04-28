#pragma once

#include <SDL.h>
#include <SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
#include <stdexcept>

#include "../gfx/shader.h"
#include "../gltf/loader.h"
#include "key.h"
#include "camera.h"
#include "gctx.h"

#include "avaterSystem.h"


class Game {
	GameContext gctx = {
		.keyStat = keyStat,
		.cam = cam
	};

	SDL_Window* window;

	bool running = true;
    uint64_t keyStat = 0; // KCode

	bgfx::ProgramHandle program;
	// bgfx::UniformHandle u_bones;
	AvaterSystem avaterSystem;

	Camera cam{WIDTH,HEIGHT};

	constexpr static int
		WIDTH = 1200,
		HEIGHT = 900;


	void gameInit();

	void onKeyDown(const SDL_KeyboardEvent& e);
    void onKeyUp(const SDL_KeyboardEvent& e);

	void update();
	void draw();
public:
	Game();
	~Game();

	void tick();
	inline bool isRunning() { return running; }
};
