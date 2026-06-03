#pragma once
#include "../gfx/mesh.h"
#include "../gfx/texture.h"
// #include "../model/model.h"
#include "../core/avatar.h"
#include "../core/str.h"
#include <tinygltf/tiny_gltf.h>
#include <utility>


struct AccessorView {
	uint8_t* data;
	size_t stride;
	size_t count;
};

class GltfLoaderImpl {
	const std::string& path;

	// in processing data
	tinygltf::Model model;

	Model scalixModel;

	void parse();
	void parseMesh(NodeId nodeId);
	void buildPalletCompress();

public:
	GltfLoaderImpl(const std::string& path);
	void load();
	inline Model get() { return std::move(scalixModel); }
};

inline Model loadGltf(const std::string& path) {
	GltfLoaderImpl loader(path);
	loader.load();
	return loader.get();
}
