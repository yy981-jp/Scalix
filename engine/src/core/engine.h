#pragma once

#if defined(_WIN32)
#include <thread>
#endif

#include <SDL3/SDL.h>
#include <bgfx/platform.h>

#include <core/bgfx.h>
#include <gfx/shader.h>
#include <gltf/loader.h>
#include <core/key.h>
#include <core/camera.h>
#include <core/gctx.h>
#include <core/grid.h>
#include <util/time.h>
#include <ui/logo.h>
#include <ui/rml.h>

#include <core/avatarSystem.h>
#include <core/nodeRegistry.h>
#include <physics/physics.h>
#include <mocap/recv.h>
#include <mocap/poseSolver.h>


enum class ShaderId {
	tex, grid,
	Count
};

enum class SceneId {
	game, ui
};

enum class EngineMode {
	normal, smokeTest, uiTest
};


class Engine {
	EngineMode emode;
	uint64_t frame = 0;

	GlobalContext gctx = {
		// .keyStat = keyStat,
		// .mStat = mStat,
		.cam = cam0,
		.cam_type = camId,
		.poseSolver = nullptr,
	};

	SDL_Window* window;
	SDL_Surface* windowIcon = nullptr;
	BGFXMan bgfxMan;

	bool running = true;
	KCodes keyStat = 0; // KCode
	bool mouseRelMode = true;
	MouseState mStat;
	ElapsedTime elap;

	SceneId sceneId = SceneId::game;
	RmlUiLayer rml;

	std::array<bgfx::ProgramHandle,static_cast<size_t>(ShaderId::Count)> shaders;
	bgfx::UniformHandle u_bones;
	int usingUni;
	AvatarSystem avatarSystem;
	PhysicsSystem physicsSystem;

	Camera cam0;
	CameraType camId;

	Grid grid;

	std::jthread mocapThr;
	Recv receiver;
	PoseSolver* poseSolver;

	int width, height;


	void gameInit();

	void onKeyDown(const SDL_KeyboardEvent& e);
	void onKeyUp(const SDL_KeyboardEvent& e);
	void onMouseBtDown(const SDL_MouseButtonEvent& e);
	void onMouseBtUp(const SDL_MouseButtonEvent& e);
	void onWindowEve(const SDL_WindowEvent& e);
	void onMouseMt(const SDL_MouseMotionEvent& e);

	void update(float dt);
	void draw();

public:
	static constexpr int
		T_WIDTH = 1200,
		T_HEIGHT = 900;

	Engine(EngineMode emode);
	~Engine();

	void tick();
	inline bool isRunning() { return running; }
};
