#pragma once

#include <gfx/mesh.h>
#include <gfx/texture.h>
#include <core/gctx.h>
#include <def/def.h>
#include <def/quat.h>
#include <def/str.h>
#include <def/transform.h>
#include <def/mtx.h>
#include <model/humanoid.h>
#include <model/rig.h>

#include <anim/format.h>
#include <core/nodeRegistry.h>


struct Node {
	StrHs name;

	NodeHandle parent;
	NodeId id = -1;
	std::vector<NodeHandle> children;

	int skinIndex = -1;
	int jointIndex = -1;

	int meshStartIndex = -1;   // scalixModel.meshesの開始インデックス
	int meshCount = 0;		 // 複数primitiveに対応するメッシュ数

	bool visible = true;

	// pos: vec3f -- local座標
	// rot: Quat  -- identity quaternion
	// scale: vec3f -- {1,1,1}
	Transform trs;
	Mtx mtx;

	Node() {
		trs.scale = {1,1,1};
	}
};

struct Skin {
	std::vector<NodeId> joints; // node index
	std::vector<Mtx> invBind;
};

enum class Status {
	stay, walk,
};


/// @brief 構造そのもの
struct Model {
	Humanoid humanoid;

	void init() {
		humanoid.init(*this);
	}

	// scalix側で用意した仮想的なroot
	// 元のデータにrootが複数ある場合も単一である場合も、
	// このnodeの子に配置する
	NodeHandle root;

	std::vector<Mesh> meshes;
	std::vector<Texture> textures;
	
	// size = 仮想rootを含む
	// => nodeHandles.size() + 1 = nodes.size()
	std::vector<Node> nodes;

	// NodeId to NodeHandle, size = 仮想rootを含まない
	std::vector<NodeHandle> nodeHandles;
	std::vector<Skin> skins;
	std::vector<int> materialToImage; // map

	RigGraph rig;
};
