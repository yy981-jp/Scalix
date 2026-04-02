#pragma once
#include "../gfx/mesh.h"
#include "../gfx/texture.h"

struct Node {
	int meshStartIndex = -1;   // scalixModel.meshesの開始インデックス
	int meshCount = 0;         // 複数primitiveに対応するメッシュ数

	float pos[3] = {0.0f, 0.0f, 0.0f};
	float rot[4] = {0.0f, 0.0f, 0.0f, 1.0f};  // identity quaternion
	float scale[3] = {1.0f, 1.0f, 1.0f};
	
	bool hasTranslation = false;
	bool hasRotation = false;
	bool hasScale = false;
};

struct Model {
	std::vector<Mesh> meshes;
	std::vector<Texture> textures;
	std::vector<Node> nodes;
	std::vector<int> materialToImage; // map
};
