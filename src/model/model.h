#pragma once

#include <string>
#include "../gfx/mesh.h"
#include "../gfx/texture.h"


struct Node {
	std::string name;

	int parent = -1;
	std::vector<int> children;

	int skinIndex = -1;

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

enum class Status {
	stay, walk,
};

enum class HumanoidBoneType {
	head,
	arm_left_up, arm_left_low,
	arm_right_up, arm_right_low,
	leg_left_up, leg_left_low,
	leg_right_up, leg_right_low,

	Count
};

struct Humanoid {
	void init(const std::vector<Node> nodes, const std::vector<Skin>& skins);

	Humanoid() {
		bones.resize(static_cast<size_t>(HumanoidBoneType::Count));
	}

	// node index
	std::vector<int> bones;
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

	// nodes mtx
	std::vector<std::array<float, 16>> globalMtxs;

	float pos[3]   = {0.0f, 0.0f, 0.0f};
	float yaw;
	float scale[3] = {1.0f, 1.0f, 1.0f};

	Status status = Status::stay;
	float speed = 0.2;
};
