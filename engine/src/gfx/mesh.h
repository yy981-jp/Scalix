#pragma once
#include <bgfx/bgfx.h>
#include <vector>

struct Vertex {
	float x,y,z;
	float nx,ny,nz;
	float u,v;

	uint16_t joints[4];
	float weights[4];

	static void init(bgfx::VertexLayout& layout) {
		layout.begin()
			.add(bgfx::Attrib::Position,3,bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal,3,bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0,2,bgfx::AttribType::Float)
			.add(bgfx::Attrib::Indices,4,bgfx::AttribType::Uint16)
			.add(bgfx::Attrib::Weight,4,bgfx::AttribType::Float)
			.end();
	}
};

struct Mesh {
	bgfx::VertexBufferHandle vbh{};
	bgfx::IndexBufferHandle ibh{};
	uint16_t indexCount{};
	int materialIndex = -1;  // glTFマテリアルインデックス

	std::vector<Vertex> verts;
	std::vector<uint16_t> indices;

	/*	元jointindex -> 圧縮後index
		@details
		boneRemap = {
			0, // Hip -> 0
			1, // Spine -> 1
			Node::invalid(),// Chest 未使用
			Node::invalid(),// Neck 未使用
			Node::invalid(),// Head 未使用
			2, // Arm_L -> 2
			Node::invalid() // Arm_R 未使用
		};
	*/
	std::vector<int> boneRemap;
	
	/*
		圧縮後index -> 元jointindex
		boneRemapInverse = {
			0, // compact 0 = Hip
			1, // compact 1 = Spine
			5  // compact 2 = Arm_L
		};
	*/
	std::vector<int> boneRemapInverse;

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
