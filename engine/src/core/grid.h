#pragma once
#include <cstdint>
#include <vector>
#include <bgfx/bgfx.h>


struct PosColorVertex {
    float x, y, z;
    uint32_t abgr;
};

class Grid {
    bgfx::VertexLayout layout;
    bgfx::VertexBufferHandle vbh;

public:
    void init(int num, float spacing, uint32_t abgr = 0xffdd963a) {
        // layout 初期化
        layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();

        std::vector<PosColorVertex> grid;

        for (int i = -num; i <= num; i++) {
            float p = i * spacing;

            // X方向の線
            grid.push_back({-num * spacing, 0.0f, p, abgr});
            grid.push_back({ num * spacing, 0.0f, p, abgr});

            // Z方向の線
            grid.push_back({p, 0.0f, -num * spacing, abgr});
            grid.push_back({p, 0.0f,  num * spacing, abgr});
        }

        const bgfx::Memory* mem = bgfx::copy(
            grid.data(),
            grid.size() * sizeof(PosColorVertex)
        );

        vbh = bgfx::createVertexBuffer(mem, layout);
    }

    void draw(bgfx::ProgramHandle program) {
        bgfx::setVertexBuffer(0, vbh);
        bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES);
        bgfx::submit(0, program);
    }
};
