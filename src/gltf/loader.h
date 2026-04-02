#pragma once
#include "../gfx/mesh.h"
#include "../gfx/texture.h"
#include <tinygltf/tiny_gltf.h>


struct Model {
	Mesh mesh;
	Texture texture;
};

class GltfLoaderImpl {
	const std::string& path;

	// in processing data
	tinygltf::Model model;
	tinygltf::Accessor posAcc;
	tinygltf::BufferView posView;
	tinygltf::Buffer posBuf;

	std::vector<Vertex> v;

	const uint8_t* posPtr;
	size_t posStride;
	const float* norPtr = nullptr;
	size_t norStride = 0;
	const float* uvPtr = nullptr;
	size_t uvStride = 0;

	Model scalixModel;

	void parse();

public:
	GltfLoaderImpl(const std::string& path);
	void load();
	inline Model get() { return scalixModel; }
};

inline Model loadGltf(const std::string& path) {
	GltfLoaderImpl loader(path);
	loader.load();
	return loader.get();
}
