#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
#include <cstdio>
#include <stdexcept>

// =====================
// Vertex定義
// =====================
struct PosColorVertex {
    float x, y, z;
    uint32_t abgr;

    static void init(bgfx::VertexLayout& layout) {
        layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();
    }
};

// =====================
// キューブデータ
// =====================
PosColorVertex vertices[] = {
    {-1,  1,  1, 0xff0000ff}, { 1,  1,  1, 0xff00ff00}, { -1, -1,  1, 0xffff0000},
    { 1, -1,  1, 0xffffffff},
    {-1,  1, -1, 0xff00ffff}, { 1,  1, -1, 0xffff00ff},
    {-1, -1, -1, 0xffffff00}, { 1, -1, -1, 0xff888888},
};

uint16_t indices[] = {
    0,1,2, 1,3,2,
    4,6,5, 5,6,7,
    0,2,4, 4,2,6,
    1,5,3, 5,7,3,
    0,4,1, 4,5,1,
    2,3,6, 6,3,7
};

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
        throw std::runtime_error("bgfx init failed\n");
        return 1;
    }

    bgfx::setViewClear(0,
        BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
        0x303030ff, 1.0f, 0);

    // バッファ
    bgfx::VertexLayout layout;
    PosColorVertex::init(layout);

    auto vbh = bgfx::createVertexBuffer(
        bgfx::makeRef(vertices, sizeof(vertices)),
        layout
    );

    auto ibh = bgfx::createIndexBuffer(
        bgfx::makeRef(indices, sizeof(indices))
    );

    auto program = loadProgram("runtime/vs_cubes.bin", "runtime/fs_cubes.bin");

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
