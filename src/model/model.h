#pragma once

#include <string>
#include "../gfx/mesh.h"
#include "../gfx/texture.h"


struct Node {
	std::string name;

	int parent = -1;
	std::vector<int> children;

	int meshStartIndex = -1;   // scalixModel.meshesの開始インデックス
	int meshCount = 0;         // 複数primitiveに対応するメッシュ数

	float pos[3] = {0.0f, 0.0f, 0.0f};
	float rot[4] = {0.0f, 0.0f, 0.0f, 1.0f};  // identity quaternion
	float scale[3] = {1.0f, 1.0f, 1.0f};

	bool hasTranslation = false;
	bool hasRotation = false;
	bool hasScale = false;
};

struct Skin {
	std::vector<int> joints; // node index
	std::vector<std::array<float,16>> invBind;
};

struct Humanoid {
	void init(const std::vector<Node> nodes, const std::vector<Skin>& skins);

	// node index
	int head = -1,
		armL = -1, armR = -1;
	
	std::vector<int> spines;
};

struct Model {
	std::vector<Mesh> meshes;
	std::vector<Texture> textures;
	std::vector<Node> nodes;
	std::vector<Skin> skins;
	std::vector<int> materialToImage; // map
};

struct Avater {
	Model model;
	Humanoid humanoid;

	std::vector<std::array<float, 16>> finalMtxs;

	float pos[3]   = {0.0f, 0.0f, 0.0f};
	float yaw;
	float scale[3] = {1.0f, 1.0f, 1.0f};

	float speed = 0.2;
};
