#include "game.h"

#include "../gfx/shader.h"

#include <iostream>

Game::Game() {
	SDL_Init(SDL_INIT_VIDEO);

	// ===== window 作成 (SDL2) =====
	window = SDL_CreateWindow(
		"Scalix",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		WIDTH, HEIGHT,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
	);

	// ===== bgfx初期化 =====
	SDL_SysWMinfo wmi;
	SDL_VERSION(&wmi.version);
	if (!SDL_GetWindowWMInfo(window, &wmi)) throw std::runtime_error("GetWMInfo");

	bgfx::PlatformData pd{};

#if defined(_WIN32)
	pd.nwh = wmi.info.win.window;
#elif defined(__linux__)
	pd.nwh = (void*)(uintptr_t)wmi.info.x11.window;
#elif defined(__APPLE__)
	pd.nwh = wmi.info.cocoa.window;
#endif

	bgfx::setPlatformData(pd);

	bgfx::Init init{};
	init.type = bgfx::RendererType::Direct3D11;

	init.platformData.nwh = pd.nwh;

	init.resolution.width = WIDTH;
	init.resolution.height = HEIGHT;
	init.resolution.reset = BGFX_RESET_VSYNC;

	if (!bgfx::init(init))
		throw std::runtime_error("bgfx init failed");

	gameInit();
}

Game::~Game() {
	bgfx::shutdown();
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void Game::tick() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT: running = false; break;
            case SDL_KEYDOWN: onKeyDown(event.key); break;
            case SDL_KEYUP: onKeyUp(event.key); break;
		}
	}

	update();

	draw();

	bgfx::frame();
}


void Game::update() {
	if (has(keyStat,KCode::Esc)) running = false;
	// ===== Entityごと =====
	if (gctx.cam_type == CameraType::DEBUG) cam.update({0.0f, 0.7f, -15},{0,0,1});
	avaterSystem.update(gctx);
}


void Game::draw() {
	avaterSystem.draw(shaders[static_cast<size_t>(ShaderId::tex)]);
	grid.draw(shaders[static_cast<size_t>(ShaderId::grid)]);
}

void Game::gameInit() {
	// set camera
	camId = CameraType::_1;

	grid.init(50,10, 0xffd6d661);

	// ===== view =====
	bgfx::setViewClear(0,
		BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
		0x303030ff, 1.0f, 0);
	bgfx::setViewRect(0, 0, 0, WIDTH, HEIGHT);

	// ===== load glTF ====
	avaterSystem.loadData({"glTF-Shinano/Shinano_AMS.gltf"});

	// ===== load Shader =====
	shaders[static_cast<size_t>(ShaderId::tex)] = loadProgram("runtime/vs_tex.bin", "runtime/fs_tex.bin");
	shaders[static_cast<size_t>(ShaderId::grid)] = loadProgram("runtime/vs_grid.bin", "runtime/fs_grid.bin");

	// const bgfx::Caps* caps = bgfx::getCaps();
	// int maxMat4 = caps->limits.maxUniforms / 4;
	// std::cout << "max: " << maxMat4 << "\n";

	// u_bones = bgfx::createUniform("u_boneMatrices", bgfx::UniformType::Mat4, 120);

}

void Game::onKeyDown(const SDL_KeyboardEvent& e) {
	if (e.repeat) return;
	auto it = keyMap.find(e.keysym.sym);
	if (it != keyMap.end())
		keyStat |= static_cast<uint64_t>(it->second);
}

void Game::onKeyUp(const SDL_KeyboardEvent& e) {
	auto it = keyMap.find(e.keysym.sym);
	if (it != keyMap.end())
		keyStat &= ~static_cast<uint64_t>(it->second);
}
