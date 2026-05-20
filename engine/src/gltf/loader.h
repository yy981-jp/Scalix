#pragma once
#include "../gfx/mesh.h"
#include "../gfx/texture.h"
// #include "../model/model.h"
#include "../core/avater.h"
#include "../core/str.h"
#include <tinygltf/tiny_gltf.h>


class GltfLoaderImpl {
	const std::string& path;

	// in processing data
	tinygltf::Model model;
	tinygltf::Accessor posAcc;
	tinygltf::BufferView posView;
	tinygltf::Buffer posBuf;

	Model scalixModel;

	void parse();
	void parseMesh(int nodeId);
	void buildPalletCompress();

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
