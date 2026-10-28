#include "def/transform.h"
#include <core/avatarSystem.h>

#include <gltf/loader.h>
#include <core/key.h>
#include <core/nodeRegistry.h>
#include <util/cache.h>
#include <def/mtx.h>
#include <def/view.h>
#include <mocap/motionDebug.h>
#include <mocap/poseSolver.h>

#include <bx/math.h>

namespace {
void traceSolverLocalRotations(Avatar& avatar, const char* stage) {
	if (!motionTraceEnabled()) return;
	for (HBT bone : {HBT::leftUpperArm, HBT::leftLowerArm, HBT::rightUpperArm, HBT::rightLowerArm}) {
		if (!avatar.model.humanoid.has(bone)) continue;
		const Quat& q = nodeReg.get(avatar.model.humanoid.bones[(size_t)bone]).trs.rot;
		printf("MOTION %s bone=%d local=(%.5f,%.5f,%.5f,%.5f)\\n", stage, static_cast<int>(bone), q.x, q.y, q.z, q.w);
	}
}
}


void AvatarSystem::loadData(const std::vector<AvatarId> ids) {
	size_t idx = 0;
	for (const auto& id: ids) {
		avatars.emplace_back(id);
		avatarIndex[id] = idx++;
	}

	playableAvatar = avatars[0].id;
}

void calcGlobal(int idx, std::vector<bool>& calculated, std::vector<Mtx>& out,
				Avatar& avatar, const Mtx& entityMtx) {
	if (idx < 0 || idx >= avatar.model.nodes.size()) {
		printf("invalid idx: %d\n", idx);
		return;
	}
	if (calculated[idx]) return;

	auto& node = avatar.model.nodes[idx];
	node.mtx = Mtx::fromTRS(node.trs);

	// 親のグローバルTRSを先に確定し、ローカルTRSを合成する。
	if (node.parent.isValid()) {
		NodeId parent = nodeReg.getId(node.parent);

		calcGlobal(parent, calculated, out, avatar, entityMtx);

		out[idx] = out[parent] * node.mtx;

	} else {
		out[idx] = node.mtx * entityMtx;
	}

	calculated[idx] = true;
}


void AvatarSystem::update(GlobalContext& ctx, float dt) {
	
	for (auto& avatar: avatars) {

		if (avatar.id != playableAvatar) continue; // player以外の制御はしない

		traceSolverLocalRotations(avatar, "beforeAvatarUpdate");
		// avatar update
		avatar.update(ctx,dt);
		traceSolverLocalRotations(avatar, "afterAvatarUpdate");

		avatar.globalMtx.resize(avatar.model.nodes.size());
		avatar.trackingMtx.resize(avatar.model.nodes.size());

		Transform entityTransform;
		entityTransform.pos = avatar.pos;
		entityTransform.rot = Quat::fromAxisAngle({0, 1, 0}, avatar.yaw);
		entityTransform.scale = avatar.scale;
		Mtx entityMtx = Mtx::fromTRS(entityTransform);

		// PoseSolver 用の階層行列。アバターのワールド位置と yaw を含めず、
		// スケールは entityTransform と同じにする。
		// トラッキング入力の基準がアバターの位置・向きに引きずられないようにする。
		Transform trackingEntityTransform;
		trackingEntityTransform.scale = entityTransform.scale;
		Mtx trackingEntityMtx = Mtx::fromTRS(trackingEntityTransform);

		std::vector<bool> calculated;
		calculated.resize(avatar.model.nodes.size(), false);

		for (int i = 0; i < avatar.model.nodes.size(); i++) {
			calcGlobal(i, calculated, avatar.globalMtx, avatar, entityMtx);
		}

		std::vector<bool> trackingCalculated;
		trackingCalculated.resize(avatar.model.nodes.size(), false);

		for (int i = 0; i < avatar.model.nodes.size(); i++) {
			calcGlobal(i, trackingCalculated, avatar.trackingMtx, avatar, trackingEntityMtx);
		}

		if (ctx.cam_type == CameraType::_1) avatar.draw(ctx.cam); // 一人称

	}
}


void AvatarSystem::draw(bgfx::ProgramHandle program, bgfx::UniformHandle u_bones) {
	for (auto& avatar: avatars) {

		// 骨構造を持たないavatarは処理しない
		if (avatar.model.skins.empty()) continue;


		std::vector<std::vector<Mtx>> allJointMtx;
		allJointMtx.resize(avatar.model.skins.size());

		for (int skinIdx = 0; skinIdx < avatar.model.skins.size(); skinIdx++) {
			const Skin& skin = avatar.model.skins[skinIdx];

			auto& jointMtx = allJointMtx[skinIdx];
			jointMtx.resize(skin.joints.size());

			for (int i = 0; i < (int)skin.joints.size(); i++) {
				NodeId nodeIdx = skin.joints[i];

				jointMtx[i] = avatar.globalMtx[nodeIdx] * skin.invBind[i];
			}
		}


		// === nodeのloop 描画loop本体とも言える ===
		for (NodeId nodeId = 0; nodeId < avatar.model.nodes.size(); nodeId++) {
			const Node& node = avatar.model.nodes[nodeId];

			// 処理する必要のないものを除外
			if (node.meshCount == 0) continue;
			if (node.skinIndex < 0) continue;
			if (!node.visible) continue;

			for (int i = 0; i < node.meshCount; i++) {
				const Mesh& mesh = avatar.model.meshes[node.meshStartIndex + i];

				// ===== パレット =====
				std::vector<Mtx> palette;
				palette.resize(mesh.boneRemapInverse.size());

				for (int i = 0; i < (int)mesh.boneRemapInverse.size(); i++) {
					int orig = mesh.boneRemapInverse[i];
					palette[i] = allJointMtx[node.skinIndex][orig];
				}

				if (mesh.boneRemapInverse.size() > 120) {
					printf("ERROR: boneRemap too big: %zu\n", mesh.boneRemap.size());
						continue;
				}
				

				bgfx::setUniform(u_bones, palette.data(), palette.size());

				// ===== transform =====
				float identity[16];
				bx::mtxIdentity(identity);
				bgfx::setTransform(identity);

				// ===== 頂点 =====
				bgfx::setVertexBuffer(0, mesh.vbh);
				bgfx::setIndexBuffer(mesh.ibh);

				// ===== テクスチャ =====
				if (mesh.materialIndex >= 0 &&
					mesh.materialIndex < (int)avatar.model.materialToImage.size()) {

					int imgIdx = avatar.model.materialToImage[mesh.materialIndex];

					if (imgIdx >= 0 &&
						imgIdx < (int)avatar.model.textures.size() &&
						avatar.model.textures[imgIdx].isValid()) {

						avatar.model.textures[imgIdx].bind();
					}
				}

				// ===== state =====
				bgfx::setState(
					BGFX_STATE_WRITE_RGB |
					BGFX_STATE_WRITE_A   |
					BGFX_STATE_WRITE_Z   |
					BGFX_STATE_DEPTH_TEST_LESS |
					BGFX_STATE_CULL_CW
				);

				// ===== draw =====
				bgfx::submit((int)ViewId::game, program);

			}

		}
	}
}

Avatar& AvatarSystem::player() {
	return avatars[avatarIndex[playableAvatar]]; // TODO: 仮
}
