#pragma once
#include <gfx/mesh.h>
#include <gfx/texture.h>
#include <core/avatar.h>
#include <def/str.h>
#include <def/node.h>
#include <tinygltf/tiny_gltf.h>
#include <utility>


struct LoadingNode {
	NodeId id;
	NodeId parent;
	std::vector<NodeId> children;
};


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

	std::vector<LoadingNode> lnodes;

	void parse();
	void parseMesh(NodeId nodeId);
	void buildPalletCompress();

	bool handleSolved = false;

public:
	GltfLoaderImpl(const std::string& path);
	void load();
	void procHdl(Avatar* avatar);
	inline Model get() {
		if (!handleSolved) throw std::runtime_error("Do not get data before solving handle");
		return std::move(scalixModel);
	}
};
