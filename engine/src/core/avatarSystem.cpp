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
	AvatarID id = 0;
	for (const auto& file: path) {
		Avatar avatar(file);
		avatar.id = id++;
		avatars.push_back(std::move(avatar));
		
		// Get a reference to the newly added avatar
		Avatar& addedAvatar = avatars.back();
		
		// Register all nodes in the registry
		for (NodeId nodeId = 0; nodeId < (NodeId)addedAvatar.model.nodes.size(); nodeId++) {
			nodeReg.create(&addedAvatar, nodeId);
		}
		
		// Now initialize humanoid with the registered nodes
		addedAvatar.humanoid.init(addedAvatar.model.nodes, addedAvatar.model.skins);
	}
}

void calcGlobal(int idx, std::vector<bool>& calculated, Avatar& avatar,
				std::vector<std::array<float,16>>& localMtxs, float* entityMtx) {
	if (calculated[idx]) return;
	if (idx < 0 || idx >= avatar.model.nodes.size()) {
		printf("invalid idx: %d\n", idx);
		return;
	}

	const auto& node = avatar.model.nodes[idx];


	// 親が先
	if (node.parent >= 0) {
		calcGlobal(node.parent, calculated, avatar, localMtxs, entityMtx);

		// 順序検証済み
		bx::mtxMul(
			avatar.globalMtxs[idx].data(),
			localMtxs[idx].data(),
			avatar.globalMtxs[node.parent].data()
		);
	
	} else {
		// ルートはentityから
		// calcGlobal(node.parent, calculated, avatar, localMtxs, entityMtx);

		bx::mtxMul(
			avatar.globalMtxs[idx].data(),
			entityMtx,
			localMtxs[idx].data()
		);
	}

	calculated[idx] = true;
}


void AvatarSystem::update(GameContext& ctx, float dt) {
	
	for (auto& avatar: avatars) {

		// printf("D: avatar.id: %d, playable: %d\n", avatar.id, playableAvatar);
		if (avatar.id != playableAvatar) continue; // player以外の制御はしない

		// avatar update
		avatar.update(ctx,dt);

		avatar.globalMtxs.clear();
		avatar.globalMtxs.resize(avatar.model.nodes.size());
		avatar.globalTransforms.clear();
		avatar.globalTransforms.resize(avatar.model.nodes.size());

		// --- Entity行列 ---
		float t[16], r[16], s[16], flip[16], tmp[16], tmp2[16], entityMtx[16];

		bx::mtxTranslate(t, avatar.pos.x, avatar.pos.y, avatar.pos.z);
		bx::mtxRotateY(r, avatar.yaw);
		bx::mtxScale(flip, -1, 1, 1);

		bx::mtxIdentity(s);
		s[0] = avatar.scale[0];  s[5] = avatar.scale[1];  s[10] = avatar.scale[2];

		bx::mtxMul(tmp, s, flip);
		bx::mtxMul(tmp2, tmp, r);
		bx::mtxMul(entityMtx, tmp2, t);


		// local mtx
		std::vector<std::array<float, 16>> localMtxs;
		localMtxs.resize(avatar.model.nodes.size());

		for (int i = 0; i < avatar.model.nodes.size(); i++) {
			const auto& node = avatar.model.nodes[i];

			buildTRS(
				localMtxs[i].data(),
				node.pos,
				node.rot,
				node.scale,
				node.hasRotation
			);
		}

		std::vector<bool> calculated;
		calculated.resize(avatar.model.nodes.size(), false);

		for (int i = 0; i < avatar.model.nodes.size(); i++) {
			calcGlobal(i,calculated,avatar,localMtxs,entityMtx);
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
					avatar.globalMtxs[nodeIdx].data()
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
