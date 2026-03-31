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

    init.platformData.nwh = (void*)wmi.info.win.window;

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
	auto res = GltfLoader::load("glTF/Shinano.gltf");

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

        bgfx::setViewRect(0, 0, 0, 800, 600);

        float view[16];
        float proj[16];

        bx::mtxLookAt(view,
            bx::Vec3{0.0f, 0.0f, -5.0f},
            bx::Vec3{0.0f, 0.0f, 0.0f}
        );

        bx::mtxProj(proj, 60.0f, 800.0f/600.0f, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);

        bgfx::setViewTransform(0, view, proj);

        float model[16];
        bx::mtxRotateY(model, time);

        bgfx::setTransform(model);

		bgfx::setVertexBuffer(0, res.mesh.vbh);
		bgfx::setIndexBuffer(res.mesh.ibh);
		
        bgfx::setState(BGFX_STATE_DEFAULT);

		bgfx::submit(0, program);

        bgfx::frame();
    }

    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
}
