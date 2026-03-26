#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
#include <cstdio>
#include <stdexcept>
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

// tinygltf
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tinygltf/tiny_gltf.h"

// =====================
// Vertex（glTF用）
// =====================
struct Vertex {
	float x, y, z;
	float nx, ny, nz;
	float u, v;

	static void init(bgfx::VertexLayout& layout) {
		layout.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal,   3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();
	}
};

// =====================
// glTFユーティリティ
// =====================
const float* getFloatData(const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
	const auto& view = model.bufferViews[accessor.bufferView];
	const auto& buffer = model.buffers[view.buffer];

	return reinterpret_cast<const float*>(
		buffer.data.data() + view.byteOffset + accessor.byteOffset
	);
}

// =====================
// shader読み込み
// =====================
bgfx::ShaderHandle loadShader(const char* path) {
	FILE* f = fopen(path, "rb");
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

	const bgfx::Memory* mem = bgfx::alloc(size + 1);
	fread(mem->data, 1, size, f);
	fclose(f);
	mem->data[mem->size - 1] = '\0';

	return bgfx::createShader(mem);
}

bgfx::ProgramHandle loadProgram(const char* vs, const char* fs) {
	auto vsh = loadShader(vs);
	auto fsh = loadShader(fs);
	return bgfx::createProgram(vsh, fsh, true);
}

// =====================
// main
// =====================
int main() {
	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window* window = SDL_CreateWindow(
		"Scalix",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		800, 600,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
	);

	// bgfxにwindowを渡す
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
	init.type = bgfx::RendererType::Count;
	init.platformData.nwh = (void*)wmi.info.win.window;

	init.resolution.width = 800;
	init.resolution.height = 600;
	init.resolution.reset = BGFX_RESET_VSYNC;

	if (!bgfx::init(init)) {
		throw std::runtime_error("bgfx init failed");
	}

	bgfx::setViewClear(0,
		BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
		0x303030ff, 1.0f, 0);

	// =====================
	// glTF読み込み
	// =====================
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;

	std::string err, warn, fname = "model.gltf";
    if (!fs::exists(fname)) throw std::runtime_error("gltf not found");
	if (!loader.LoadASCIIFromFile(&model, &err, &warn, fname)) throw std::runtime_error("gltf load failed");

	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	const auto& mesh = model.meshes[0];
	const auto& prim = mesh.primitives[0];

	// position
	const auto& posAcc = model.accessors[prim.attributes.find("POSITION")->second];
	const float* pos = getFloatData(model, posAcc);

	// normal
	const auto& norAcc = model.accessors[prim.attributes.find("NORMAL")->second];
	const float* nor = getFloatData(model, norAcc);

	// uv
	const auto& uvAcc = model.accessors[prim.attributes.find("TEXCOORD_0")->second];
	const float* uv = getFloatData(model, uvAcc);

	// 頂点生成
	for (size_t i = 0; i < posAcc.count; i++) {
		Vertex v{};

		v.x = pos[i * 3 + 0];
		v.y = pos[i * 3 + 1];
		v.z = pos[i * 3 + 2];

		v.nx = nor[i * 3 + 0];
		v.ny = nor[i * 3 + 1];
		v.nz = nor[i * 3 + 2];

		v.u = uv[i * 2 + 0];
		v.v = uv[i * 2 + 1];

		vertices.push_back(v);
	}

	// index
	const auto& idxAcc = model.accessors[prim.indices];
	const auto& idxView = model.bufferViews[idxAcc.bufferView];
	const auto& idxBuf = model.buffers[idxView.buffer];

	const uint16_t* idx = reinterpret_cast<const uint16_t*>(
		idxBuf.data.data() + idxView.byteOffset + idxAcc.byteOffset
	);

	for (size_t i = 0; i < idxAcc.count; i++) {
		indices.push_back(idx[i]);
	}

	// =====================
	// bgfxバッファ作成
	// =====================
	bgfx::VertexLayout layout;
	Vertex::init(layout);

	auto vbh = bgfx::createVertexBuffer(
		bgfx::makeRef(vertices.data(), vertices.size() * sizeof(Vertex)),
		layout
	);

	auto ibh = bgfx::createIndexBuffer(
		bgfx::makeRef(indices.data(), indices.size() * sizeof(uint16_t))
	);

	auto program = loadProgram("runtime/vs_cubes.bin", "runtime/fs_cubes.bin");

	// =====================
	// メインループ
	// =====================
	bool running = true;
	float time = 0.0f;

	while (running) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) running = false;
		}

		time += 0.01f;

		bgfx::setViewRect(0, 0, 0, 800, 600);

		float view[16];
		float proj[16];

		bx::mtxLookAt(view,
			bx::Vec3{0.0f, 0.0f, -5.0f},
			bx::Vec3{0.0f, 0.0f, 0.0f}
		);

		bx::mtxProj(proj, 60.0f, 800.0f/600.0f, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);

		bgfx::setViewTransform(0, view, proj);

		float modelMtx[16];
		bx::mtxRotateY(modelMtx, time);

		bgfx::setTransform(modelMtx);
		bgfx::setVertexBuffer(0, vbh);
		bgfx::setIndexBuffer(ibh);

		bgfx::setState(BGFX_STATE_DEFAULT);

		bgfx::submit(0, program);

		bgfx::frame();
	}

	bgfx::shutdown();
	SDL_DestroyWindow(window);
	SDL_Quit();
}
