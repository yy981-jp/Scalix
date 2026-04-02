#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
#include <stdexcept>

#include "gltf/loader.h"

#include <filesystem>

namespace fs = std::filesystem;

// shader読み込み
bgfx::ShaderHandle loadShader(const char* path) {
	if (!fs::exists(path)) throw std::runtime_error(std::string{"Shader not found: "} + path);

	FILE* f = fopen(path, "rb");
	if (!f) throw std::runtime_error("fopen failed");
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

    if (size == 0) throw std::runtime_error("shader: size = 0");

	const bgfx::Memory* mem = bgfx::alloc(size/* + 1*/);
	fread(mem->data, 1, size, f);
	fclose(f);
	// mem->data[mem->size - 1] = '\0';

	return bgfx::createShader(mem);
}

bgfx::ProgramHandle loadProgram(const char* vs, const char* fs) {
	auto vsh = loadShader(vs);
	auto fsh = loadShader(fs);
	return bgfx::createProgram(vsh, fsh, true);
}

int main() {
	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window* window = SDL_CreateWindow(
		"Scalix",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		800, 600,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
	);

	// =====================
	// bgfx初期化
	// =====================
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

	bgfx::setViewClear(0,
		BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
		0x303030ff, 1.0f, 0);

	// =====================
	// glTFロード
	// =====================
	auto res = loadGltf("glTF/Shinano.gltf");
	// auto res = loadGltf("glTF-boots/boots.gltf");
	// auto res = GltfLoader::load("glTF-cube/cube.gltf");

	printf("=== Model Data ===\n");
	printf("Meshes: %zu\n", res.meshes.size());
	printf("Nodes: %zu\n", res.nodes.size());
	printf("Textures: %zu\n", res.textures.size());
	
	for (size_t i = 0; i < res.nodes.size(); i++) {
		printf("Node[%zu]: meshIdx=%d, pos=(%f,%f,%f), scale=(%f,%f,%f)\n",
			i, res.nodes[i].meshIndex,
			res.nodes[i].pos[0], res.nodes[i].pos[1], res.nodes[i].pos[2],
			res.nodes[i].scale[0], res.nodes[i].scale[1], res.nodes[i].scale[2]);
		printf("          rot=(%f,%f,%f,%f)\n",
			res.nodes[i].rot[0], res.nodes[i].rot[1], res.nodes[i].rot[2], res.nodes[i].rot[3]);
	}
	
	for (size_t i = 0; i < res.meshes.size(); i++) {
		printf("Mesh[%zu]: materialIdx=%d\n", i, res.meshes[i].materialIndex);
	}
	printf("==================\n\n");

	// shader（UV対応のやつ必要）
	auto program = loadProgram("runtime/vs_tex.bin", "runtime/fs_tex.bin");

	// =====================
	// ループ
	// =====================
    bool running = true;
    float time = 0.0f;

    while (running) {
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
			bx::Vec3{0.0f, 0.0f, -3.0f},
			bx::Vec3{0.0f, 0.0f, 0.0f}
		);

		bx::mtxProj(proj, 60.0f, 800.0f/600.0f, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);

		bgfx::setViewTransform(0, view, proj);

		// ===== ノードごと描画 =====
		for (const auto& node : res.nodes) {

			if (node.meshIndex < 0 || node.meshIndex >= static_cast<int>(res.meshes.size())) {
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

			// --- 描画 ---
			const Mesh& m = res.meshes[node.meshIndex];

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

			bgfx::setState(BGFX_STATE_DEFAULT);

			bgfx::submit(0, program);
		}

        bgfx::frame();
    }

    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
}
