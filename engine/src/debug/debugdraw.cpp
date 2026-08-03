#include <debug/debugDraw.h>
#include <bgfx/bgfx.h>

DebugDraw debug;

DebugDraw::DebugDraw() {
}

void DebugDraw::init() {
	layout.begin()
		.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
		.end();

	points.reserve(8192);
	colors.reserve(8192);
}

void DebugDraw::drawLine(const vec3f& a, const vec3f& b, uint32_t abgr) {
	points.push_back(a);
	colors.push_back(abgr);
	points.push_back(b);
	colors.push_back(abgr);
}

void DebugDraw::drawCross(const vec3f& position, float size, uint32_t abgr) {
	const vec3f x{size, 0.0f, 0.0f};
	const vec3f y{0.0f, size, 0.0f};
	const vec3f z{0.0f, 0.0f, size};

drawLine(position - x, position + x, abgr);

drawLine(position - y, position + y, abgr);

drawLine(position - z, position + z, abgr);
}

void DebugDraw::render(bgfx::ProgramHandle program) {
	if (points.empty()) {
		return;
	}

	const uint32_t numVertices = static_cast<uint32_t>(points.size());
	bgfx::TransientVertexBuffer tvb;

	bgfx::allocTransientVertexBuffer(&tvb, numVertices, layout);

	struct PosColorVertex {
		float x, y, z;
		uint32_t abgr;
	};

	PosColorVertex* dst = reinterpret_cast<PosColorVertex*>(tvb.data);

	for (uint32_t i = 0; i < numVertices; ++i) {
		dst[i].x = points[i].x;
		dst[i].y = points[i].y;
		dst[i].z = points[i].z;
		dst[i].abgr = colors[i];
	}

	bgfx::setVertexBuffer(0, &tvb);
	bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_LESS);
	bgfx::submit(0, program);

	reset();
}

void DebugDraw::reset() {
	points.clear();
	colors.clear();
}
