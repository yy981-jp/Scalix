#include <bx/math.h>

#include <def/transform.h>
#include <core/avatarSystem.h>
#include <loader/gltf.h>
#include <core/key.h>
#include <core/nodeRegistry.h>
#include <util/cache.h>
#include <def/mtx.h>
#include <def/view.h>
#include <mocap/motionDebug.h>
#include <mocap/poseSolver.h>


void AvatarSystem::loadData(AvatarId id) {
	// Avatarのコンストラクタがload処理をやってる
	avatars.try_emplace(id, id);

	if (!playableAvatar.isValid())
		playableAvatar = id;
}


void AvatarSystem::update(GlobalContext& ctx, float dt) {
	// player以外の制御はしない
	Avatar& avatar = player();

	// avatar update
	avatar.update(ctx,dt);

	avatar.globalMtx.resize(avatar.model.nodes.size());
	avatar.avatarSpaceMtx.resize(avatar.model.nodes.size());

	Mtx trackingEntityMtx;
	trackingEntityMtx.setScale(avatar.avatarMtx.scale());

	avatar.refreshSubtreeFor(avatar.avatarSpaceMtx,trackingEntityMtx);

	Mtx trackingToGlobal =
		avatar.avatarMtx * Mtx::inverse(trackingEntityMtx);

	for (size_t i = 0; i < avatar.model.nodes.size(); i++) {
		avatar.globalMtx[i] = trackingToGlobal * avatar.avatarSpaceMtx[i];
	}

	if (ctx.cam_type == CameraType::_1) avatar.draw(ctx.cam); // 一人称
}


void AvatarSystem::draw(bgfx::ProgramHandle program, bgfx::UniformHandle u_bones) {
	for (auto& [id,avatar]: avatars) {

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
	return get(playableAvatar);
}

Avatar& AvatarSystem::get(const AvatarId& id) {
	return avatars.at(id);
}
