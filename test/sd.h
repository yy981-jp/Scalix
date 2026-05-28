#include <SDL.h>
#include <SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>

struct PosVertex {
	float x, y, z;
	static bgfx::VertexLayout layout;
};

bgfx::VertexLayout PosVertex::layout;

static bgfx::ShaderHandle loadShader(const char* path) {
	std::ifstream file(path, std::ios::binary);
	if(!file) {
		std::cerr << "shader load failed: " << path << '\n';
		return BGFX_INVALID_HANDLE;
	}

	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	file.seekg(0);

	const bgfx::Memory* mem = bgfx::alloc(uint32_t(size + 1));
	file.read((char*)mem->data, size);
	mem->data[size] = '\0';

	return bgfx::createShader(mem);
}

int main() {
	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window* win = SDL_CreateWindow(
		"Shader Test",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		800, 600,
		SDL_WINDOW_SHOWN
	);

	SDL_SysWMinfo wmi;
	SDL_VERSION(&wmi.version);
	SDL_GetWindowWMInfo(win, &wmi);

	bgfx::Init init;
	init.type = bgfx::RendererType::Count;
	init.platformData.nwh = wmi.info.win.window;

	bgfx::init(init);
	bgfx::reset(800, 600, BGFX_RESET_VSYNC);

	PosVertex::layout
		.begin()
		.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
		.end();

	const PosVertex verts[] = {
		{-1.0f, -1.0f, 0.0f},
		{ 1.0f, -1.0f, 0.0f},
		{-1.0f,  1.0f, 0.0f},
		{ 1.0f,  1.0f, 0.0f},
	};

	const uint16_t indices[] = {
		0,1,2,
		1,3,2
	};

	auto vbh = bgfx::createVertexBuffer(
		bgfx::copy(verts, sizeof(verts)),
		PosVertex::layout
	);

	auto ibh = bgfx::createIndexBuffer(
		bgfx::copy(indices, sizeof(indices))
	);

	auto vsh = loadShader("runtime/vs_test.bin");
	auto fsh = loadShader("runtime/fs_test.bin");
	auto prog = bgfx::createProgram(vsh, fsh, true);

	auto u_time = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);

	bool run = true;
	SDL_Event e;

	while(run) {
		while(SDL_PollEvent(&e)) {
			if(e.type == SDL_QUIT)
				run = false;
		}

		float time[4] = {
			SDL_GetTicks() / 1000.0f,
			0,0,0
		};

		bgfx::setViewClear(0,
			BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
			0x303030ff);

		bgfx::touch(0);

		bgfx::setUniform(u_time, time);
		bgfx::setVertexBuffer(0, vbh);
		bgfx::setIndexBuffer(ibh);
		bgfx::submit(0, prog);

		bgfx::frame();
	}

	bgfx::shutdown();
	SDL_DestroyWindow(win);
	SDL_Quit();
}
