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
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT: running = false; break;
            case SDL_KEYDOWN: onKeyDown(event.key); break;
            case SDL_KEYUP: onKeyUp(event.key); break;
		}
	}

	time += 0.01f;

	update();

	draw();

	bgfx::frame();
}


void Game::update() {
	// ===== Entityごと =====
	for (auto& res: avaters) {
		res.finalMtxs.clear();

		// --- Entityの変換行列 ---
		float entityMtx[16];
		float tE[16], rE[16], sE[16];

		bx::mtxIdentity(tE);
		bx::mtxIdentity(rE);
		bx::mtxIdentity(sE);

		// translation
		tE[12] = res.pos[0];
		tE[13] = res.pos[1];
		tE[14] = res.pos[2];

		// rotation
		{
			bx::Quaternion q = {
				res.rot[0],
				res.rot[1],
				res.rot[2],
				res.rot[3]
			};
			bx::mtxFromQuaternion(rE, q);
		}

		// scale
		sE[0]  = res.scale[0];
		sE[5]  = res.scale[1];
		sE[10] = res.scale[2];

		float tmpE[16];
		bx::mtxIdentity(tmpE);
		bx::mtxIdentity(entityMtx);
		bx::mtxMul(tmpE, rE, sE);        // R * S
		bx::mtxMul(entityMtx, tE, tmpE); // T * (R * S)

		// ===== ノードごと更新 =====
		for (const auto& node : res.model.nodes) {

			if (node.meshStartIndex < 0 || node.meshCount <= 0) continue;

			// --- nodeの変換 ---
			float nodeMtx[16];
			float t[16], r[16], s[16];

			// Initialize matrices
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

			float tmp[16];
			bx::mtxIdentity(tmp);
			bx::mtxIdentity(nodeMtx);
			bx::mtxMul(tmp, r, s);
			bx::mtxMul(nodeMtx, t, tmp);

			// --- 方向 ---
			float anim[16];
			bx::mtxIdentity(anim);
			bx::mtxRotateY(anim, 3.9f);
			bx::mtxMul(tmp, nodeMtx, anim);

			// --- 移動 ---
			float move[16];
			bx::mtxIdentity(move);
			float z = 0.f;
			if (has(keyStat,KCode::W)) z = -0.5;

			bx::mtxTranslate(move, 0.0f, 0.0f, z);
			bx::mtxMul(tmp, tmp, move);

			// ★ Entityを最後に掛ける（これが一番重要）
			std::array<float,16> final;
			std::fill(final.begin(), final.end(), 0.0f);  // または bx::mtxIdentity の後に mtxMul を使う
			bx::mtxMul(final.data(), entityMtx, tmp);
			res.finalMtxs.push_back(final);
		}
	}
}

void Game::draw() {
	// ===== カメラ =====
	float view[16];
	float proj[16];

	bx::mtxLookAt(view,
		bx::Vec3{0.0f, 0.7f, -1.5f},
		bx::Vec3{0.0f, 0.7f, 0.0f}
	);

	bx::mtxProj(proj, 60.0f, 800.0f/600.0f, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);

	bgfx::setViewTransform(0, view, proj);


	// ===== ノードごと描画 =====
	for (auto& res: avaters) {
		int mtxIdx = 0;
		for (int nodeIdx = 0; nodeIdx < res.model.nodes.size(); nodeIdx++) {
			const auto& node = res.model.nodes[nodeIdx];

			if (node.meshStartIndex < 0 || node.meshCount <= 0) continue;

			// === 複数primitiveを描画 ===
			for (int i = 0; i < node.meshCount; i++) {
				const Mesh& m = res.model.meshes[node.meshStartIndex + i];

				bgfx::setTransform(res.finalMtxs[mtxIdx].data());

				bgfx::setVertexBuffer(0, m.vbh);
				bgfx::setIndexBuffer(m.ibh);

				// テクスチャバインド（マテリアル → イメージ → テクスチャ）
				if (m.materialIndex >= 0 && m.materialIndex < static_cast<int>(res.model.materialToImage.size())) {
					int imgIdx = res.model.materialToImage[m.materialIndex];
					if (imgIdx >= 0 && imgIdx < static_cast<int>(res.model.textures.size()) && res.model.textures[imgIdx].isValid()) {
						res.model.textures[imgIdx].bind();
					}
				}

				bgfx::setState(
					BGFX_STATE_WRITE_RGB		|
					BGFX_STATE_WRITE_A  		|
					BGFX_STATE_WRITE_Z			|
					BGFX_STATE_DEPTH_TEST_LESS	|
					BGFX_STATE_CULL_CCW
				);

				bgfx::submit(0, program);
			}
			mtxIdx++;
		}
	}
}

void Game::gameInit() {
	// ===== view =====
	bgfx::setViewClear(0,
		BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
		0x303030ff, 1.0f, 0);
	bgfx::setViewRect(0, 0, 0, 800, 600);

	// ===== load glTF ====
	avaters.push_back( loadEntity("glTF/Shinano.gltf") );
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
