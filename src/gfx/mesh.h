#pragma once
#include <bgfx/bgfx.h>
#include <vector>

struct Vertex {
	float x,y,z;
	float nx,ny,nz;
	float u,v;

	static void init(bgfx::VertexLayout& layout) {
		layout.begin()
			.add(bgfx::Attrib::Position,3,bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal,3,bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0,2,bgfx::AttribType::Float)
			.end();
	}
};

struct Mesh {
	bgfx::VertexBufferHandle vbh{};
	bgfx::IndexBufferHandle ibh{};
	uint16_t indexCount{};

	void create(const std::vector<Vertex>& v, const std::vector<uint16_t>& i) {
		bgfx::VertexLayout layout;
		Vertex::init(layout);

		vbh = bgfx::createVertexBuffer(
			bgfx::copy(v.data(), v.size()*sizeof(Vertex)),
			layout
		);

		ibh = bgfx::createIndexBuffer(
			bgfx::copy(i.data(), i.size()*sizeof(uint16_t))
		);

		indexCount = (uint16_t)i.size();
	}

	void submit(bgfx::ProgramHandle program) {
		bgfx::setVertexBuffer(0, vbh);
		bgfx::setIndexBuffer(ibh);
		bgfx::submit(0, program);
	}
};
