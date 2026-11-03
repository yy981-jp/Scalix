#include <physics/springBone.h>
#include <core/avatar.h>


SpringBoneChain::SpringBoneChain(
	const std::vector<NodeHandle>& nodes,
	const std::vector<SpringBoneParams>& params
):
	nodes(nodes), params(params), bones(nodes.size()),
	restLengths(nodes.size() > 1 ? nodes.size() - 1 : 0),
	restDirections(restLengths.size()) {

	// Initialize positions from nodes
	for (size_t i = 0; i < bones.size(); i++) {
		Avatar* ava = nodeReg.getAvatar(nodes[i]);
		const auto& mtx = ava->globalMtx[nodeReg.getId(nodes[i])];

		bones[i].currPos = mtx.pos();
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


void SpringBoneChain::update(float dt) {
	if (bones.size() < 2) return;

	// Keep the root anchored to its animated world-space position.
	Avatar* rootAvatar = nodeReg.getAvatar(nodes[0]);
	const auto& rootMtx = rootAvatar->globalMtx[nodeReg.getId(nodes[0])];
	bones[0].currPos = rootMtx.pos();
	bones[0].prevPos = bones[0].currPos;

	// 予測: 速度 + 重力
	for (size_t i = 1; i < nodes.size(); ++i) {
		auto& tail = bones[i];
		const auto& head = bones[i - 1];
		const auto& param = params[i - 1];

		// 慣性
		vec3f inertia =
			(tail.currPos - tail.prevPos) * (1.0f - param.dragForce);

		tail.prevPos = tail.currPos;

		// 初期姿勢に戻ろうとする力
		// // parentWorldRotation * initialLocalRotation * boneAxis
		// vec3f restDir = getRestDirectionWorld(head);
		// vec3f stiffness = restDir * (param.stiffness * dt);

		// 重力
		vec3f gravity = param.gravityDir * (param.gravityPower * dt);

		// tail.currPos += inertia + stiffness + gravity;
	}

	// 制約: ボーン長を保つ
	for (int i = 1; i < (int)bones.size(); ++i) {
		auto& parent = bones[i - 1];
		auto& child  = bones[i];
		NodeHandle h = nodes[i];

		vec3f dir = child.currPos - parent.currPos;
		float len = bx::length(dir);

		dir = len > 0.001f ? dir / len : restDirections[i - 1];

		child.currPos = parent.currPos + dir * restLengths[i - 1];

		// Convert the global simulation position back to the parent's local space.
		Avatar* parentAvatar = nodeReg.getAvatar(nodes[i - 1]);
		Mtx parentMtx =
			parentAvatar->globalMtx[nodeReg.getId(nodes[i - 1])];
		parentMtx.setPos(parent.currPos);

		Mtx invParentMtx;
		// bx::mtxInverse(invParentMtx, parentMtx.data());
		invParentMtx = Mtx::inverse(parentMtx);
		nodeReg.get(h).trs.pos = child.currPos * invParentMtx;
	}
}
