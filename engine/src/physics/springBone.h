#pragma once
#include <vector>

#include <def/def.h>
#include <def/vec3.h>
#include <model/model.h>

#include <physics/def.h>


struct SpringBoneNode {
	vec3f prevPos;
	vec3f currPos; // 直前の位置
};

class SpringBoneChain {
	std::vector<NodeId> boneIndex; // 鎖の基点から末端まで順に並べる
	std::vector<SpringBoneNode> bones;
	std::vector<float> restLengths; // bone間の長さ

	float stiffness;
	float drag;

public:

	SpringBoneChain(const std::vector<NodeId>& boneIndex):
	  boneIndex(boneIndex), bones(boneIndex.size()), restLengths(boneIndex.size()-1) {}

	
	void update(float dt, std::vector<Node>& nodes) {
		for (int i = 0; i <= boneIndex.size(); i++) {
			auto& bone = bones[i];
			const NodeId& id = boneIndex[i];

			vec3f vel = bone.currPos - bone.prevPos;
			bone.prevPos = bone.currPos;
			
			bone.currPos += vel;
			bone.currPos += def::gravity * dt * dt;
			
			
		}
	}
};


class SpringBoneSystem {
	std::vector<SpringBoneChain> chains;

public:

	void add(const SpringBoneChain& chain) {
		chains.push_back(chain);
	}

	void update(float dt, std::vector<Node> nodes) {
		for (auto& chain: chains) {
			chain.update(dt, nodes);
		}
	}

};
