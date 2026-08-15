#include <physics/springBone.h>
#include <core/avatar.h>


SpringBoneChain::SpringBoneChain(const std::vector<NodeHandle>& boneIndex):
	stiffness(0.5f), drag(0.5f),
	boneIndex(boneIndex), bones(boneIndex.size()),
	restLengths(boneIndex.size() > 1 ? boneIndex.size() - 1 : 0),
	restDirections(restLengths.size()) {
	// Initialize positions from nodes
	for (size_t i = 0; i < bones.size(); i++) {
		Avatar* ava = nodeReg.getAvatar(boneIndex[i]);
		const auto& mtx = ava->globalMtx[nodeReg.getId(boneIndex[i])];

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
	Avatar* rootAvatar = nodeReg.getAvatar(boneIndex[0]);
	const auto& rootMtx = rootAvatar->globalMtx[nodeReg.getId(boneIndex[0])];
	bones[0].currPos = rootMtx.pos();
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
		Mtx parentMtx =
			parentAvatar->globalMtx[nodeReg.getId(boneIndex[i - 1])];
		parentMtx.setPos(parent.currPos);

		Mtx invParentMtx;
		// bx::mtxInverse(invParentMtx, parentMtx.data());
		invParentMtx = Mtx::inverse(parentMtx);
		nodeReg.get(h).trs.pos = child.currPos * invParentMtx;
	}
}
