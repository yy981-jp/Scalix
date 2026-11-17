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
		std::vector<SpringBoneCollider> colliders;
		for (const auto& grIdx: spring.colliderGroups) {
			const auto& group = springBone.colliderGroups[grIdx.value];

			for (const auto& colIdx: group.colliders) {
				const auto& source = springBone.colliders[colIdx.value];

				SpringBoneCollider col;
				col.node = avatar.model.nodeHandles[source.node.value];

				std::visit([&](const auto& shape) {
					using T = std::decay_t<decltype(shape)>;

					col.offset = {
						shape.offset[0],
						shape.offset[1],
						shape.offset[2]
					};
					col.radius = shape.radius;

					if constexpr (std::is_same_v<T, vrm10::CapsuleCollider>) {
						col.shape = ColliderShape::Capsule;
						col.tail = {
							shape.tail[0],
							shape.tail[1],
							shape.tail[2]
						};
					} else {
						col.shape = ColliderShape::Sphere;
					}
				}, source.shape);

				colliders.push_back(col);
			}
		}


		NodeHandle center = spring.center.has_value()? 
			avatar.model.nodeHandles[spring.center->value] : 
			NodeHandle::invalid();


		// 登録
		springBoneSys.add(SpringBoneChain(
			avatar,
			nodes,
			params,
			center,
			colliders
		));
	}
}
