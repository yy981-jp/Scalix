#pragma once

#include <array>
#include <gfx/mesh.h>
#include <gfx/texture.h>
#include <core/gctx.h>
#include <def/def.h>
#include <def/quat.h>
#include <def/str.h>
#include <def/transform.h>

#include <anim/format.h>
#include <core/nodeRegistry.h>


struct Node {
	StrHs name;

	NodeHandle parent;
	NodeId id = -1;
	std::vector<NodeHandle> children;

	int skinIndex = -1;

	int meshStartIndex = -1;   // scalixModel.meshesの開始インデックス
	int meshCount = 0;		 // 複数primitiveに対応するメッシュ数

	bool visible = true;

	// pos: vec3f -- local座標
	// rot: Quat  -- identity quaternion
	// scale: vec3f
	Transform trs;

	Node() {
		trs.scale = {1,1,1};
	}
};

struct Skin {
	std::vector<NodeId> joints; // node index
	std::vector<std::array<float,16>> invBind;
};

enum class Status {
	stay, walk,
};


/// @brief 構造そのもの
struct Model {
	std::vector<Mesh> meshes;
	std::vector<Texture> textures;
	std::vector<Node> nodes;
	std::vector<NodeHandle> nodeHandles; // NodeId to NodeHandle
	std::vector<Skin> skins;
	std::vector<int> materialToImage; // map
};
