#include "game.h"

#include "../gfx/shader.h"

Game::Game() {
	SDL_Init(SDL_INIT_VIDEO);

	// ===== window 作成 (SDL2) =====
	window = SDL_CreateWindow(
		"Scalix",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		800, 600,
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

	init.resolution.width = 800;
	init.resolution.height = 600;
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
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) running = false;
	}

	time += 0.01f;

	// ===== カメラ =====
	bgfx::setViewRect(0, 0, 0, 800, 600);

	float view[16];
	float proj[16];

	bx::mtxLookAt(view,
		bx::Vec3{0.0f, 0.7f, -1.5f},
		bx::Vec3{0.0f, 0.7f, 0.0f}
	);

	bx::mtxProj(proj, 60.0f, 800.0f/600.0f, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);

	bgfx::setViewTransform(0, view, proj);

	// ===== ノードごと描画 =====
	for (const auto& node : res.nodes) {

		if (node.meshStartIndex < 0 || node.meshCount <= 0) {
			continue;
		}

		// --- nodeの変換行列 ---
		float nodeMtx[16];
		float t[16], r[16], s[16];

		bx::mtxIdentity(t);
		bx::mtxIdentity(r);
		bx::mtxIdentity(s);

		// translation
		t[12] = node.pos[0];
		t[13] = node.pos[1];
		t[14] = node.pos[2];

		// rotation
		if (node.hasRotation) {
			bx::Quaternion q = {
				node.rot[0],
				node.rot[1],
				node.rot[2],
				node.rot[3]
			};
			bx::mtxFromQuaternion(r, q);
		}

		// scale
		s[0]  = node.scale[0];
		s[5]  = node.scale[1];
		s[10] = node.scale[2];

		// 合成
		float tmp[16];
		bx::mtxMul(tmp, r, s);     // R * S
		bx::mtxMul(nodeMtx, t, tmp); // T * (R * S)

		// --- 自転（時間回転） ---
		float anim[16];
		bx::mtxRotateY(anim, time);

		float final[16];
		bx::mtxMul(final, nodeMtx, anim);

		// === 複数primitiveを描画 ===
		for (int i = 0; i < node.meshCount; i++) {
			const Mesh& m = res.meshes[node.meshStartIndex + i];

			bgfx::setTransform(final);

			bgfx::setVertexBuffer(0, m.vbh);
			bgfx::setIndexBuffer(m.ibh);

			// テクスチャバインド（マテリアル → イメージ → テクスチャ）
			if (m.materialIndex >= 0 && m.materialIndex < static_cast<int>(res.materialToImage.size())) {
				int imgIdx = res.materialToImage[m.materialIndex];
				if (imgIdx >= 0 && imgIdx < static_cast<int>(res.textures.size()) && res.textures[imgIdx].isValid()) {
					res.textures[imgIdx].bind();
				}
			}

			bgfx::setState(
				BGFX_STATE_WRITE_RGB	|
				BGFX_STATE_WRITE_A  	|
				BGFX_STATE_WRITE_Z		|
				BGFX_STATE_DEPTH_TEST_LESS
			);

			bgfx::submit(0, program);
		}
	}

	bgfx::frame();
}

void Game::gameInit() {
	bgfx::setViewClear(0,
		BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
		0x303030ff, 1.0f, 0);
	// ===== load glTF ====
	res = loadGltf("glTF/Shinano.gltf");
	// ===== load Shader =====
	program = loadProgram("runtime/vs_tex.bin", "runtime/fs_tex.bin");
}