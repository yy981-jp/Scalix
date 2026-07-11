#pragma once

#include <SDL.h>
#include <SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
#include <stdexcept>

#include <gfx/shader.h>
#include <gltf/loader.h>
#include <core/key.h>
#include <core/camera.h>
#include <core/gctx.h>
#include <core/grid.h>
#include <util/time.h>
#include <ui/logo.h>

#include <core/avatarSystem.h>
#include <core/nodeRegistry.h>
#include <physics/springBone.h>


enum class ShaderId {
	tex, grid,
	Count
};

class Game {
	GameContext gctx = {
		.keyStat = keyStat,
		.mStat = mStat,
		.cam = cam0,
		.cam_type = camId
	};

	SDL_Window* window;

	bool running = true;
    KCodes keyStat = 0; // KCode
	bool mouseRelMode = true;
	MouseState mStat;
	ElapsedTime elap;

	std::array<bgfx::ProgramHandle,static_cast<size_t>(ShaderId::Count)> shaders;
	bgfx::UniformHandle u_bones;
	int usingUni;
	AvatarSystem avatarSystem;

	SpringBoneSystem springBoneSystem;
	NodeRegistry nodeReg;

	LogoRenderer* logo;

	Camera cam0;
	CameraType camId;

	Grid grid;

	static constexpr int
		T_WIDTH = 1200,
		T_HEIGHT = 900;

	int width, height;


	void gameInit();

	void onKeyDown(const SDL_KeyboardEvent& e);
    void onKeyUp(const SDL_KeyboardEvent& e);
	void onMouseBtDown(const SDL_MouseButtonEvent& e);
	void onMouseBtUp(const SDL_MouseButtonEvent& e);
	void onWindowEve(const SDL_WindowEvent& e);
	void onMouseMt(const SDL_MouseMotionEvent& e);

	void update();
	void draw();

public:
	Game();
	~Game();

	void tick();
	inline bool isRunning() { return running; }
};
