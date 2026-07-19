#pragma once
#include <algorithm>
#include <array>
#include <vector>

#include <def/def.h>
#include <def/vec3.h>
#include <model/model.h>

#include <physics/def.h>
#include <core/nodeRegistry.h>


inline std::vector<NodeHandle> detectBonePath(NodeHandle begin, NodeHandle end) {
	std::vector<NodeHandle> path;

	if (!nodeReg.is_alive(begin) || !nodeReg.is_alive(end))
		return path;

	Avatar* avatar = nodeReg.getAvatar(begin);
	if (avatar != nodeReg.getAvatar(end))
		return path;

	const std::vector<Node>& nodes = avatar->model.nodes;
	const NodeId beginId = nodeReg.getId(begin);
	NodeId cur = nodeReg.getId(end);
	std::vector<bool> visited(nodes.size(), false);
	bool foundBegin = false;

	while (cur != -1) {
		if (cur < 0 || static_cast<size_t>(cur) >= nodes.size() || visited[cur])
			return {};
		visited[cur] = true;

		NodeHandle handle = nodeReg.find(avatar->id, cur);
		if (!nodeReg.is_alive(handle))
			return {};
		path.push_back(handle);

		if (cur == beginId) {
			foundBegin = true;
			break;
		}

		cur = nodes[cur].parent;
	}

	if (!foundBegin)
		return {};

	std::reverse(path.begin(), path.end());

	return path;
}


struct SpringBoneNode {
	vec3f prevPos;
	vec3f currPos; // 直前の位置
};

class SpringBoneChain {
	std::vector<NodeHandle> boneIndex; // 鎖の基点から末端まで順に並べる
	std::vector<SpringBoneNode> bones;
	std::vector<float> restLengths; // bone間の長さ
	std::vector<vec3f> restDirections;
	float stiffness = 0.95f;
	float drag = 0.99f;

public:

	SpringBoneChain(const std::vector<NodeHandle>& boneIndex):
	  boneIndex(boneIndex), bones(boneIndex.size()),
	  restLengths(boneIndex.size() > 1 ? boneIndex.size() - 1 : 0),
	  restDirections(restLengths.size()) {
		// Initialize positions from nodes
		for (size_t i = 0; i < bones.size(); i++) {
			Avatar* ava = nodeReg.getAvatar(boneIndex[i]);
			const auto& mtx = ava->globalTransforms[nodeReg.getId(boneIndex[i])].mtx;

			bones[i].currPos = {mtx[12], mtx[13], mtx[14]};
			bones[i].prevPos = bones[i].currPos;
		}
		
		// Calculate rest lengths
		for (size_t i = 0; i < restLengths.size(); i++) {
			vec3f delta = bones[i + 1].currPos - bones[i].currPos;
			restLengths[i] = bx::length(delta);
			restDirections[i] = restLengths[i] > 0.001f
				? delta / restLengths[i]
				: vec3f{0.0f, -1.0f, 0.0f};
		}
	}

	
	void update(float dt) {
		if (bones.size() < 2) return;

		// Keep the root anchored to its animated world-space position.
		Avatar* rootAvatar = nodeReg.getAvatar(boneIndex[0]);
		const auto& rootMtx = rootAvatar->globalTransforms[nodeReg.getId(boneIndex[0])].mtx;
		bones[0].currPos = {rootMtx[12], rootMtx[13], rootMtx[14]};
		bones[0].prevPos = bones[0].currPos;

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

			dir = len > 0.001f ? dir / len : restDirections[i - 1];

			child.currPos = parent.currPos + dir * restLengths[i - 1];

			// Convert the global simulation position back to the parent's local space.
			Avatar* parentAvatar = nodeReg.getAvatar(boneIndex[i - 1]);
			std::array<float, 16> parentMtx =
				parentAvatar->globalTransforms[nodeReg.getId(boneIndex[i - 1])].mtx;
			parentMtx[12] = parent.currPos.x;
			parentMtx[13] = parent.currPos.y;
			parentMtx[14] = parent.currPos.z;

			float invParentMtx[16];
			bx::mtxInverse(invParentMtx, parentMtx.data());
			nodeReg.get(h).trs.pos = vec3f{bx::mul(child.currPos, invParentMtx)};
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
