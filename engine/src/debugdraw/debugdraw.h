#pragma once

#include <bgfx/bgfx.h>
#include <def/vec3.h>
#include <cstdint>
#include <vector>

struct DebugDraw {
	DebugDraw();

	void init();
	void drawLine(const vec3f& a, const vec3f& b, uint32_t abgr = 0xffffffffu);
	void drawCross(const vec3f& position, float size, uint32_t abgr = 0xffffffffu);
	void render(bgfx::ProgramHandle program);
	void reset();

	bool isEmpty() const { return points.empty(); }

private:
	bgfx::VertexLayout layout;
	std::vector<vec3f> points;
	std::vector<uint32_t> colors;
};

extern DebugDraw debugDraw;
