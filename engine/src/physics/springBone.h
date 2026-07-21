#pragma once
#include <algorithm>
#include <array>
#include <vector>

#include <def/def.h>
#include <def/vec3.h>
#include <model/model.h>

#include <physics/def.h>
#include <core/nodeRegistry.h>


struct SpringBoneNode {
	vec3f prevPos;
	vec3f currPos; // 直前の位置
};

class SpringBoneChain {
	std::vector<NodeHandle> boneIndex; // 鎖の基点から末端まで順に並べる
	std::vector<SpringBoneNode> bones;
	std::vector<float> restLengths; // bone間の長さ
	std::vector<vec3f> restDirections;
	float stiffness; // 元に戻ろうとする力
	float drag; // 空気抵抗

public:

	SpringBoneChain(const std::vector<NodeHandle>& boneIndex);
	void update(float dt);

};


class SpringBoneSystem {
	std::vector<SpringBoneChain> chains;

public:

	void add(const SpringBoneChain& chain) {
		chains.push_back(chain);
	}

	void update(float dt) {
		for (auto& chain: chains) {
			chain.update(dt);
		}
	}

};
