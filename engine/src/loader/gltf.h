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

	void parse();
	void parseMesh(NodeId nodeId);
	void buildPalletCompress();

	bool handleSolved = false;

public:
	GltfLoader(const std::string& path, GltfType type);
	void load();
	const std::vector<std::string>& getExtensionsUsed() const { return model.extensionsUsed; }
	json getExtension(const std::string& str) const {
		const auto it = model.extensions.find(str);

		if (it == model.extensions.end()) throw std::runtime_error("Not found extension");
		return convertToJson(it->second);
	}
	json getGltf() const {
		tinygltf::TinyGLTF writer;
		std::ostringstream stream;

		if (!writer.WriteGltfSceneToStream(
			&model,
			stream,
			false,
			false
		)) {
			throw std::runtime_error("Failed to serialize glTF");
		}

		return json::parse(stream.str());

	}
	void procHdl(Avatar* avatar);
	inline Model get() {
		if (!handleSolved) throw std::runtime_error("Do not get data before solving handle.");
		return std::move(scalixModel);
	}
};
