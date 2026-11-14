#include <algorithm>
#include <cstdio>
#include <unordered_set>

#include <physics/physics.h>
#include <loader/data.h>


void PhysicsSystem::init(Avatar& avatar, const LoadData& ld) {
	springBoneSys.clear();

	const auto& springBone = ld.vrm.springBone;

	std::vector<NodeHandle> nodes;
	std::vector<SpringBoneParams> params;
	for (const auto& spring: springBone.springs) {
		int jointSize = spring.joints.size();
		// シミュレーション可能なチェーンか確認
		if (jointSize < 2) continue;

		nodes.clear();	nodes.reserve(jointSize);
		params.clear();	params.reserve(jointSize);

		for (const auto& joint: spring.joints) {
			// NodeIndex -> NodeHandle
			nodes.push_back(avatar.model.nodeHandles[joint.node.value]);
			// VRMデータ -> 物理パラメータ
			SpringBoneParams param;
			param.stiffness = joint.stiffness;
			param.dragForce = joint.dragForce;
			param.gravityPower = joint.gravityPower;
			param.gravityDir = joint.gravityDir;
			param.hitRadius = joint.hitRadius;
			params.push_back(param);
		}

		// ColliderGroup -> Physics Collider
		auto colliders = resolveColliders(avatar, springBone, spring);

		// 登録
		springBoneSys.add(SpringBoneChain(
			avatar,
			nodes,
			params,
			resolveCenter(avatar, spring),
			colliders
		));
	}
}


PhysicsSystem phSys;
