#pragma once
#include <utility>

#include <tinygltf/tiny_gltf.h>

#include <gfx/mesh.h>
#include <gfx/texture.h>
#include <loader/jsonConv.h>
#include <core/avatar.h>
#include <def/str.h>
#include <def/node.h>


struct LoadingNode {
	NodeId id;
	NodeId parent;
	std::vector<NodeId> children;
};


enum class GltfType {
	ascii,
	binary
};

class GltfLoader {
	const std::string& path;
	GltfType type;

	// in processing data
	tinygltf::Model model;

	Model scalixModel;
	std::vector<LoadingNode> lnodes;
	json vrmJson;

	void parse();
	void parseMesh(NodeId nodeId);
	void buildPalletCompress();

	bool handleSolved = false;

public:
	GltfLoader(const std::string& path, GltfType type);
	void load();
	const json& extensions() {
		const auto it = model.extensions.find("VRMC_vrm");

		if (it == model.extensions.end()) throw std::runtime_error("This is not vrm data.");
		vrmJson = convertToJson(it->second);
		return vrmJson;
	}
	void procHdl(Avatar* avatar);
	inline Model get() {
		if (!handleSolved) throw std::runtime_error("Do not get data before solving handle.");
		return std::move(scalixModel);
	}
};
