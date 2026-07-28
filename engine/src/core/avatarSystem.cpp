#include <core/avatarSystem.h>

#include <gltf/loader.h>
#include <core/key.h>
#include <core/nodeRegistry.h>
#include <util/mtxutil.h>
#include <util/cache.h>
#include <util/quatutil.h>

#include <bx/math.h>
#include <iostream>


void AvatarSystem::loadData(const std::vector<std::string> path) {
	AvatarId id = 0;
	for (const auto& file: path) {
		// NodeRegistry stores the Avatar* passed during Avatar construction.
		// Construct the Avatar in its final container so that this address does
		// not become stale after moving a local Avatar into `avatars`.
		avatars.emplace_back(file, id++);
	}
}

void calcGlobal(int idx, std::vector<bool>& calculated, Avatar& avatar,
				const Transform& entityTransform) {
	if (idx < 0 || idx >= avatar.model.nodes.size()) {
		printf("invalid idx: %d\n", idx);
		return;
	}
	if (calculated[idx]) return;

	auto& node = avatar.model.nodes[idx];
	node.trs.rebuildMatrix();

	// 親のグローバルTRSを先に確定し、ローカルTRSを合成する。
	if (node.parent.isValid()) {
		NodeId parent = nodeReg.getId(node.parent);
		calcGlobal(parent, calculated, avatar, entityTransform);
		avatar.globalTransforms[idx] = avatar.globalTransforms[parent] * node.trs;
	} else {
		// bx::mtxMul の従来の積順と合わせ、ルートは local * entity。
		avatar.globalTransforms[idx] = node.trs * entityTransform;
	}

	calculated[idx] = true;
}


void AvatarSystem::update(GameContext& ctx, float dt) {
	
	for (auto& avatar: avatars) {

		// printf("D: avatar.id: %d, playable: %d\n", avatar.id, playableAvatar);
		if (avatar.id != playableAvatar) continue; // player以外の制御はしない

		// avatar update
		avatar.update(ctx,dt);

		avatar.globalTransforms.resize(avatar.model.nodes.size());

		Transform entityTransform;
		entityTransform.pos = avatar.pos;
		entityTransform.rot.setAxisAngle({0, 1, 0}, avatar.yaw);
		// glTF座標系からのX軸反転もEntityのTRSとして扱う。
		entityTransform.scale = {-avatar.scale[0], avatar.scale[1], avatar.scale[2]};
		entityTransform.rebuildMatrix();

		std::vector<bool> calculated;
		calculated.resize(avatar.model.nodes.size(), false);

		for (int i = 0; i < avatar.model.nodes.size(); i++) {
			calcGlobal(i, calculated, avatar, entityTransform);
		}

		if (ctx.cam_type == CameraType::_1) avatar.draw(ctx.cam); // 一人称

	}
}


void AvatarSystem::draw(bgfx::ProgramHandle program, bgfx::UniformHandle u_bones) {
	for (auto& avatar: avatars) {

		// 骨構造を持たないavatarは処理しない
		if (avatar.model.skins.empty()) continue;


		std::vector<std::vector<std::array<float,16>>> allJointMtx;
		allJointMtx.resize(avatar.model.skins.size());

		for (int skinIdx = 0; skinIdx < avatar.model.skins.size(); skinIdx++) {
			const Skin& skin = avatar.model.skins[skinIdx];

			auto& jointMtx = allJointMtx[skinIdx];
			jointMtx.resize(skin.joints.size());

			for (int i = 0; i < (int)skin.joints.size(); i++) {
				NodeId nodeIdx = skin.joints[i];

				// 順序検証済み
				bx::mtxMul(
					jointMtx[i].data(),
					skin.invBind[i].data(),
					avatar.globalTransforms[nodeIdx].mtx.data()
				);
			}
		}


		// === nodeのloop 描画loop本体とも言える ===
		for (NodeId nodeId = 0; nodeId < avatar.model.nodes.size(); nodeId++) {
			const Node& node = avatar.model.nodes[nodeId];

			// 処理する必要のないものを除外
			if (node.meshCount == 0) continue;
			if (node.skinIndex < 0) continue;
			if (!node.visible) continue;

			// printf("DEBUG: node name: %s\n", std::string(strsv().get(node.name)).c_str());

			for (int i = 0; i < node.meshCount; i++) {
				const Mesh& mesh = avatar.model.meshes[node.meshStartIndex + i];

				// ===== パレット =====
				std::vector<std::array<float,16>> palette;
				palette.resize(mesh.boneRemapInverse.size());

				for (int i = 0; i < (int)mesh.boneRemapInverse.size(); i++) {
					int orig = mesh.boneRemapInverse[i];
					palette[i] = allJointMtx[node.skinIndex][orig];
				}

				if (mesh.boneRemapInverse.size() > 120) {
					printf("ERROR: boneRemap too big: %zu\n", mesh.boneRemap.size());
					// printf("rem-inv: %d", mesh.boneRemapInverse.size());
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
				bgfx::submit(0, program);

				// printf("DEBUG: size: %d\n", mesh.boneRemapInverse.size());
			}

		}
	}
}

Avatar& AvatarSystem::player() {
	return avatars[playableAvatar];
}
