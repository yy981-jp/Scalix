#pragma once
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

struct SpringBoneParams {
	float stiffness;
	float dragForce;
	float gravityPower;
	vec3f gravityDir;
	float hitRadius;
};

class SpringBoneChain {
	std::vector<NodeHandle> nodes; // 鎖の基点から末端まで順に並べる
	std::vector<SpringBoneNode> bones;
	std::vector<float> restLengths; // bone間の長さ
	std::vector<vec3f> restDirections;

	std::vector<SpringBoneParams> params;

public:
	SpringBoneChain(
		const std::vector<NodeHandle>& nodes,
		const std::vector<SpringBoneParams>& params
	);
	void update(float dt);

};


class SpringBoneSystem {
	std::vector<SpringBoneChain> chains;

public:

	void add(const SpringBoneChain& chain) {
		chains.push_back(chain);
	}

	void tick(float dt) {
		for (auto& chain: chains) {
			chain.update(dt);
		}
	}

};
