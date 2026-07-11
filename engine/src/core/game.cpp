#include <core/game.h>

#include <gfx/shader.h>

#include <iostream>

Game::Game() {
	SDL_Init(SDL_INIT_VIDEO);
	if(!(IMG_Init(IMG_INIT_WEBP) & IMG_INIT_WEBP)) return;

	// ===== window 作成 (SDL2) =====
	window = SDL_CreateWindow(
		"Scalix",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		T_WIDTH, T_HEIGHT,
		SDL_WINDOW_FULLSCREEN_DESKTOP
	);

	// get window size
	SDL_GetWindowSize(window, &width, &height);

	// logo system
	logo = new LogoRenderer(window);
	logo->draw();

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

	init.resolution.width = width;
	init.resolution.height = height;
	init.resolution.reset = BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X4/* | BGFX_RESET_MAXANISOTROPY*/;

	
	if (!bgfx::init(init)) throw std::runtime_error("bgfx init failed");
	
	bgfx::setDebug(BGFX_DEBUG_STATS | BGFX_DEBUG_TEXT);

	// set mouse mode

	SDL_SetRelativeMouseMode(static_cast<SDL_bool>(mouseRelMode));

	gameInit();
}

Game::~Game() {
	SDL_DestroyWindow(window);
	SDL_Quit();
	IMG_Quit();
}

void Game::tick() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT: running = false; break;
            case SDL_KEYDOWN: onKeyDown(event.key); break;
            case SDL_KEYUP: onKeyUp(event.key); break;
			case SDL_MOUSEBUTTONDOWN: onMouseBtDown(event.button); break;
			case SDL_MOUSEBUTTONUP: onMouseBtUp(event.button); break;
			case SDL_WINDOWEVENT: onWindowEve(event.window); break;
			case SDL_MOUSEMOTION: onMouseMt(event.motion); break;
		}
	}

	update();
	
	draw();
	
	// TODO: 何故動かない...?
	bgfx::dbgTextClear();
	bgfx::dbgTextPrintf(10, 25, 0x4f, "y9test: debug hello");


	bgfx::frame();
}


void Game::update() {
	if (has(keyStat,KCode::Esc)) running = false;
    float dt = elap.get();

	// ===== Entityごと =====
	if (gctx.cam_type == CameraType::DEBUG) cam0.update({0.0f, 0.7f, -15},{0.0f, 0.7f, 0});
	avatarSystem.update(gctx,dt);
	springBoneSystem.update(dt);

	mStat.relPos = {0, 0};
}


void Game::draw() {
	avatarSystem.draw(shaders[static_cast<size_t>(ShaderId::tex)], u_bones);
	grid.draw(shaders[static_cast<size_t>(ShaderId::grid)]);
}

void Game::gameInit() {
	mStat.relMode = mouseRelMode;

	// set camera
	camId = CameraType::_1;
	cam0.init(width,height,0.03);

	grid.init(50,10, 0xffd6d661);

	// ===== view =====
	bgfx::setViewClear(0,
		BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
		0x303030ff, 1.0f, 0);
	bgfx::setViewRect(0, 0, 0, width, height);

	// ===== load glTF ====
	avatarSystem.loadData({"glTF-Shinano/Shinano_AMS.gltf"});
	// avatarSystem.loadData({"glTF-Sponza/Sponza.gltf"});

	// ===== load Shader =====
	shaders[static_cast<size_t>(ShaderId::tex)] = loadProgram("runtime/vs_tex.bin", "runtime/fs_tex.bin");
	shaders[static_cast<size_t>(ShaderId::grid)] = loadProgram("runtime/vs_grid.bin", "runtime/fs_grid.bin");

	// const bgfx::Caps* caps = bgfx::getCaps();
	// int maxMat4 = caps->limits.maxUniforms / 4;
	// usingUni = maxMat4 * 0.8;
	// std::cout << "using uniform: " << usingUni << "\n";

	u_bones = bgfx::createUniform("u_boneMatrices", bgfx::UniformType::Mat4, 120);

	elap.init();

	delete logo;


	springBoneSystem.add(SpringBoneChain({nodeReg.nd_find(176),nodeReg.nd_find(113),nodeReg.nd_find(112),nodeReg.nd_find(111)/*,110*/})); // back d l
	springBoneSystem.add(SpringBoneChain({nodeReg.nd_find(176),nodeReg.nd_find(105),nodeReg.nd_find(104),nodeReg.nd_find(103)/*,102*/})); // back c l
	springBoneSystem.add(SpringBoneChain({nodeReg.nd_find(176),nodeReg.nd_find(120),nodeReg.nd_find(119),nodeReg.nd_find(118)})); // back e l
}

void Game::onKeyDown(const SDL_KeyboardEvent& e) {
	if (e.repeat) return;
	auto it = keyMap.find(e.keysym.sym);
	if (it != keyMap.end())
		keyStat |= static_cast<KCodes>(it->second);
}

void Game::onKeyUp(const SDL_KeyboardEvent& e) {
	auto it = keyMap.find(e.keysym.sym);
	if (it != keyMap.end())
		keyStat &= ~static_cast<KCodes>(it->second);
}

void Game::onMouseBtDown(const SDL_MouseButtonEvent& e) {
	auto it = mMap.find(e.button);
	if (it != mMap.end())
		mStat.codes |= static_cast<MCodes>(it->second);
}

void Game::onMouseBtUp(const SDL_MouseButtonEvent& e) {
	auto it = mMap.find(e.button);
	if (it != mMap.end())
		mStat.codes &= ~static_cast<MCodes>(it->second);
}

void Game::onWindowEve(const SDL_WindowEvent& e) {
/*
	if (e.event == SDL_WINDOWEVENT_FOCUS_LOST) {
		// windowがfocusを失ったとき入力を初期化 ...した方がいいのか? TODO:
	}
*/
}

void Game::onMouseMt(const SDL_MouseMotionEvent& e) {
	mStat.absPos.x = e.x;
	mStat.absPos.y = e.y;
	mStat.relPos.x = e.xrel;
	mStat.relPos.y = e.yrel;
}
