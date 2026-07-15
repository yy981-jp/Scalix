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

class SpringBoneChain {
	std::vector<NodeHandle> boneIndex; // 鎖の基点から末端まで順に並べる
	std::vector<SpringBoneNode> bones;
	std::vector<float> restLengths; // bone間の長さ

	float stiffness = 0.95f;
	float drag = 0.99f;

public:

	SpringBoneChain(const std::vector<NodeHandle>& boneIndex):
	  boneIndex(boneIndex), bones(boneIndex.size()), restLengths(boneIndex.size()-1) {
		// Initialize positions from nodes
		for (int i = 0; i < bones.size(); i++) {
			bones[i].currPos = nodeReg.get(boneIndex[i]).trs.pos;
			bones[i].prevPos = bones[i].currPos;
		}
		
		// Calculate rest lengths
		for (int i = 0; i < (int)bones.size()-1; i++) {
			restLengths[i] = bx::distance(
				nodeReg.get(boneIndex[i]).trs.pos,
				nodeReg.get(boneIndex[i + 1]).trs.pos
			);
		}
	}

	
	void update(float dt) {
		// 予測: 速度 + 重力
		for (int i = 1; i < (int)boneIndex.size(); i++) {
			auto& bone = bones[i];

			vec3f vel = (bone.currPos - bone.prevPos) * drag;
			bone.prevPos = bone.currPos;
			
			bone.currPos += vel;
			bone.currPos += def::gravity * dt * dt;
		}

		// 制約: ボーン長を保つ
		for (int i = 1; i < (int)bones.size(); ++i) {
			auto& parent = bones[i - 1];
			auto& child  = bones[i];
			NodeHandle h = boneIndex[i];

			vec3f dir = child.currPos - parent.currPos;
			float len = bx::length(dir);

			if (len > 0.001f) {
				dir = dir / len;
			}

			child.currPos = parent.currPos + dir * restLengths[i - 1];
			nodeReg.get(h).trs.pos = child.currPos;
		}
	}
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
