#pragma once
#include <tinygltf/tiny_gltf.h>
#include "../gfx/mesh.h"
#include "../gfx/texture.h"

namespace gltfLoader {

struct Result {
	Mesh mesh;
	Texture texture;
};

const float* getFloat(const tinygltf::Model& m, const tinygltf::Accessor& a) {
	const auto& view = m.bufferViews[a.bufferView];
	const auto& buf  = m.buffers[view.buffer];

	return reinterpret_cast<const float*>(
		buf.data.data() + view.byteOffset + a.byteOffset
	);
}

Result load(const char* path) {
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;

	std::string err, warn;
	if (!loader.LoadASCIIFromFile(&model, &err, &warn, path))
		throw std::runtime_error("gltf load failed");

	Result r;

	const auto& prim = model.meshes[0].primitives[0];

	// --- position ---
	const auto& posAcc = model.accessors[prim.attributes.find("POSITION")->second];
	const float* pos = getFloat(model, posAcc);

	// --- normal ---
	const auto& norAcc = model.accessors[prim.attributes.find("NORMAL")->second];
	const float* nor = getFloat(model, norAcc);

	// --- uv ---
	const auto& uvAcc = model.accessors[prim.attributes.find("TEXCOORD_0")->second];
	const float* uv = getFloat(model, uvAcc);

	std::vector<Vertex> v;
	for (size_t i=0;i<posAcc.count;i++) {
		Vertex vert{};
		vert.x = pos[i*3+0];
		vert.y = pos[i*3+1];
		vert.z = pos[i*3+2];

		vert.nx = nor[i*3+0];
		vert.ny = nor[i*3+1];
		vert.nz = nor[i*3+2];

		vert.u = uv[i*2+0];
		vert.v = uv[i*2+1];

		v.push_back(vert);
	}

	// index
	std::vector<uint16_t> idx;
	const auto& ia = model.accessors[prim.indices];
	const auto& iv = model.bufferViews[ia.bufferView];
	const auto& ib = model.buffers[iv.buffer];

	const uint16_t* data = reinterpret_cast<const uint16_t*>(
		ib.data.data() + iv.byteOffset + ia.byteOffset
	);

	for (size_t i=0;i<ia.count;i++) idx.push_back(data[i]);

	r.mesh.create(v, idx);

	// texture（最初の1枚）
	if (!model.images.empty()) {
		const auto& img = model.images[0];
		r.texture.create(
			img.width, img.height,
			img.component,
			img.image.data(),
			img.image.size()
		);
	}

	return r;
}

}