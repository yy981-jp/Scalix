#include "game.h"

#include "../gfx/shader.h"

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
	init.type = bgfx::RendererType::Direct3D12;

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
	avaterSystem.update(keyStat);
}


void Game::draw() {
	// ===== カメラ =====
	float view[16];
	float proj[16];

	bx::mtxLookAt(view,
		bx::Vec3{0.0f, 0.7f, -15},
		bx::Vec3{0.0f, 0.7f, 0.f}
	);

	bx::mtxProj(proj, 60.0f, SceneAspect, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);

	bgfx::setViewTransform(0, view, proj);

	avaterSystem.draw(program);
}

void Game::gameInit() {
	// ===== view =====
	bgfx::setViewClear(0,
		BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
		0x303030ff, 1.0f, 0);
	bgfx::setViewRect(0, 0, 0, WIDTH, HEIGHT);

	// ===== load glTF ====
	avaterSystem.loadData({"glTF-Shinano/Shinano_AMS.gltf"});
	// ===== load Shader =====
	program = loadProgram("runtime/vs_tex.bin", "runtime/fs_tex.bin");
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
