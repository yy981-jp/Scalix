#include <core/engine.h>

#include <mocap/mocap.h>

#include <stdexcept>
#include <bx/math.h>

#include <gfx/shader.h>
#include <debug/debugDraw.h>
#include <util/path.h>
#include <core/config.h>
#include <physics/util.h>


Engine::Engine() {
	SDL_Init(SDL_INIT_VIDEO);
	// if(!(IMG_Init(IMG_INIT_WEBP) & IMG_INIT_WEBP)) return;

	// ===== window 作成 (SDL) =====
	window = SDL_CreateWindow(
		"Scalix",
		// SDL_WINDOWPOS_CENTERED,
		// SDL_WINDOWPOS_CENTERED,
		T_WIDTH, T_HEIGHT,
		0
		// SDL_WINDOW_FULLSCREEN
		// SDL_WINDOW_FULLSCREEN_DESKTOP
	);
	SDL_SetWindowFullscreen(window, true);

	// path system
	syspath.init();

	// logo system
	LogoRenderer logo(window);
	logo.draw();

	// get window size
	SDL_GetWindowSize(window, &width, &height);

	// ===== bgfx初期化 =====
	bgfx::PlatformData pd{};

	SDL_PropertiesID props = SDL_GetWindowProperties(window);

	#if defined(_WIN32)
		pd.nwh = SDL_GetPointerProperty(
			props,
			SDL_PROP_WINDOW_WIN32_HWND_POINTER,
			nullptr
		);
	#elif defined(__linux__)
		pd.nwh = reinterpret_cast<void*>(
			SDL_GetNumberProperty(
				props,
				SDL_PROP_WINDOW_X11_WINDOW_NUMBER,
				0
			)
		);
	#elif defined(__APPLE__)
		pd.nwh = SDL_GetPointerProperty(
			props,
			SDL_PROP_WINDOW_COCOA_WINDOW_POINTER,
			nullptr
		);
	#endif

	if (!pd.nwh)
		throw std::runtime_error("Get native window handle");

	// bgfx::setPlatformData(pd);

	bgfx::Init init{};
	init.type = bgfx::RendererType::Count;

	init.platformData.nwh = pd.nwh;

	init.resolution.width = width;
	init.resolution.height = height;
	init.resolution.reset = BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X4/* | BGFX_RESET_MAXANISOTROPY*/;

	
	if (!bgfx::init(init)) throw std::runtime_error("bgfx init failed");
	
	bgfx::setDebug(BGFX_DEBUG_STATS);
	// bgfx::setDebug(BGFX_DEBUG_TEXT);

	// set mouse mode

	SDL_SetWindowRelativeMouseMode(window, mouseRelMode);

	cfg.init((syspath.userdata/"config"/"user.json").string(), (syspath.resource/"config"/"def.json").string());
	gameInit();

	gctx.poseSolver = poseSolver;

}

Engine::~Engine() {
	SDL_DestroyWindow(window);
	SDL_Quit();
	// IMG_Quit();
}

void Engine::tick() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_EVENT_QUIT: running = false; break;
			case SDL_EVENT_KEY_DOWN: onKeyDown(event.key); break;
			case SDL_EVENT_KEY_UP: onKeyUp(event.key); break;
			case SDL_EVENT_MOUSE_BUTTON_DOWN: onMouseBtDown(event.button); break;
			case SDL_EVENT_MOUSE_BUTTON_UP: onMouseBtUp(event.button); break;
			// case SDL_WINDOWEVENT: onWindowEve(event.window); break;
			case SDL_EVENT_MOUSE_MOTION: onMouseMt(event.motion); break;
		}
	}

	const float dt = elap.get();
	update(dt);
	
	draw();

	bgfx::dbgTextClear();
	bgfx::dbgTextPrintf(5, 50, 0x4f, "fps: %f", 1/dt);


	bgfx::frame();
}


void Engine::update(float dt) {
	if (has(keyStat,KCode::Esc)) running = false;

	// ===== Entityごと =====
	if (gctx.cam_type == CameraType::DEBUG) cam0.update({0.0f, 0.7f, -5},{0.0f, 0.7f, 0});
	springBoneSystem.update(dt);

	PoseFrame frame;
	if (receiver.tick(frame)) {
		poseSolver->solve(frame);
	}

	avatarSystem.update(gctx,dt);

	mStat.relPos = {0, 0};
}


void Engine::draw() {
	avatarSystem.draw(shaders[static_cast<size_t>(ShaderId::tex)], u_bones);
	grid.draw(shaders[static_cast<size_t>(ShaderId::grid)]);
	debug.render(shaders[static_cast<size_t>(ShaderId::grid)]);
}

void Engine::gameInit() {
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
	avatarSystem.loadData({
		UUID::fromString(
			cfg.get()["avatar"]["id"].get<std::string>()
		)
	});
	// avatarSystem.loadData({"glTF-Sponza/Sponza.gltf"});

	// ===== load Shader =====
	shaders[static_cast<size_t>(ShaderId::tex)] = loadProgram(
		(syspath.resource / "runtime/vs_tex.bin").string(),
		(syspath.resource / "runtime/fs_tex.bin").string()
	);
	shaders[static_cast<size_t>(ShaderId::grid)] = loadProgram(
		(syspath.resource / "runtime/vs_grid.bin").string(),
		(syspath.resource / "runtime/fs_grid.bin").string()
	);
	
	debug.init();

	// const bgfx::Caps* caps = bgfx::getCaps();
	// int maxMat4 = caps->limits.maxUniforms / 4;
	// usingUni = maxMat4 * 0.8;
	// std::cout << "using uniform: " << usingUni << "\n";

	u_bones = bgfx::createUniform("u_boneMatrices", bgfx::UniformType::Mat4, 120);

	elap.init();


	avatarSystem.update(gctx,elap.get());


	poseSolver = new PoseSolver{avatarSystem.player()};

	
	// for (const NodeHandle& nh: nodeReg.get(nodeReg.findFromAvaId(0, 176)).children ) {
	// 	springBoneSystem.add(SpringBoneChain( detectBonePath(nh) ));
	// }

	springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatarSystem.player().id,113)) )); // back d l
	springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatarSystem.player().id,117)) )); // back d r
	springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatarSystem.player().id,105)) )); // back c l
	springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatarSystem.player().id,109)) )); // back c r
	springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatarSystem.player().id,120)) )); // back e l
	springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatarSystem.player().id,123)) )); // back e r
	springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatarSystem.player().id,98)) )); // back a
	springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatarSystem.player().id,101)) )); // back b
	springBoneSystem.add(SpringBoneChain( detectBonePath(nodeReg.findFromAvaId(avatarSystem.player().id,199)) )); // 

	if (cfg.get()["mocap"].get<bool>())
		mocapThr = std::jthread(mocapWorker);
}

void Engine::onKeyDown(const SDL_KeyboardEvent& e) {
	if (e.repeat) return;
	auto it = keyMap.find(e.key);
	if (it != keyMap.end())
		keyStat |= static_cast<KCodes>(it->second);
}

void Engine::onKeyUp(const SDL_KeyboardEvent& e) {
	auto it = keyMap.find(e.key);
	if (it != keyMap.end())
		keyStat &= ~static_cast<KCodes>(it->second);
}

void Engine::onMouseBtDown(const SDL_MouseButtonEvent& e) {
	auto it = mMap.find(e.button);
	if (it != mMap.end())
		mStat.codes |= static_cast<MCodes>(it->second);
}

void Engine::onMouseBtUp(const SDL_MouseButtonEvent& e) {
	auto it = mMap.find(e.button);
	if (it != mMap.end())
		mStat.codes &= ~static_cast<MCodes>(it->second);
}

// void Engine::onWindowEve(const SDL_WindowEvent& e) {
// /*
// 	if (e.event == SDL_EVENT_WINDOW_FOCUS_LOST) {
// 		// windowがfocusを失ったとき入力を初期化 ...した方がいいのか? TODO:
// 	}
// */
// }

void Engine::onMouseMt(const SDL_MouseMotionEvent& e) {
	mStat.absPos.x = e.x;
	mStat.absPos.y = e.y;
	mStat.relPos.x = e.xrel;
	mStat.relPos.y = e.yrel;
}
