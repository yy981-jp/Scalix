#include "avaterSystem.h"

#include "../gltf/loader.h"
#include "key.h"
#include "mtxutil.h"
#include "cache.h"

#include <bx/math.h>


void AvaterSystem::loadData(const std::vector<std::string> path) {
	for (const auto& file: path) {
		avaters.push_back( loadEntity(file) );
        auto& avater = avaters.back();
        avater.humanoid.init(avater.model.nodes, avater.model.skins);
    }
}


void AvaterSystem::update(const uint64_t& keyStat) {
    for (auto& avater : avaters) {
        avater.finalMtxs.clear();

        // 向き操作
        if (has(keyStat, KCode::A)) avater.yaw += 0.05f;
        if (has(keyStat, KCode::D)) avater.yaw -= 0.05f;

        // 移動 → yawから直接XZ成分を計算してpos更新
        if (has(keyStat, KCode::W)) {
            avater.pos[0] -= cachesv.getSin(avater.yaw) * avater.speed;
            avater.pos[2] += cachesv.getCos(avater.yaw) * avater.speed;
        }
        if (has(keyStat, KCode::S)) {
            avater.pos[0] += cachesv.getSin(avater.yaw) * avater.speed;
            avater.pos[2] -= cachesv.getCos(avater.yaw) * avater.speed;
        }

        // --- Entity行列（T * R * S のみ）---
        float t[16], r[16], s[16], tmp[16], entityMtx[16];

        bx::mtxTranslate(t, avater.pos[0], avater.pos[1], avater.pos[2]);
        bx::mtxRotateY(r, avater.yaw);

        bx::mtxIdentity(s);
        s[0] = avater.scale[0];  s[5] = avater.scale[1];  s[10] = avater.scale[2];

        bx::mtxMul(tmp, s, r);
        bx::mtxMul(entityMtx, tmp, t);

        // --- ノードごと（nodeMtxはメッシュオフセットのみ）---
        for (const auto& node: avater.model.nodes) {
            if (node.meshStartIndex < 0 || node.meshCount <= 0) continue;

            float nodeMtx[16];
            buildTRS(nodeMtx, node.pos, node.rot, node.scale, node.hasRotation);

            // entity * node のみ。移動・向きはentityMtxに全部入ってる
            std::array<float, 16> finalMtx;
            bx::mtxMul(finalMtx.data(), entityMtx, nodeMtx);
            avater.finalMtxs.push_back(finalMtx);
        }

        // --- bone 更新 ---
        for (const auto& skin: avater.model.skins) {
            // TODO
        }
    }
}

void AvaterSystem::draw(bgfx::ProgramHandle program) {
	// ===== ノードごと描画 =====
	for (auto& res: avaters) {
		int mtxIdx = 0;
		for (int nodeIdx = 0; nodeIdx < res.model.nodes.size(); nodeIdx++) {
			const auto& node = res.model.nodes[nodeIdx];

			if (node.meshStartIndex < 0 || node.meshCount <= 0) continue;

			// === 複数primitiveを描画 ===
			for (int i = 0; i < node.meshCount; i++) {
				const Mesh& m = res.model.meshes[node.meshStartIndex + i];

				bgfx::setTransform(res.finalMtxs[mtxIdx].data());

				bgfx::setVertexBuffer(0, m.vbh);
				bgfx::setIndexBuffer(m.ibh);

				// テクスチャバインド（マテリアル → イメージ → テクスチャ）
				if (m.materialIndex >= 0 && m.materialIndex < static_cast<int>(res.model.materialToImage.size())) {
					int imgIdx = res.model.materialToImage[m.materialIndex];
					if (imgIdx >= 0 && imgIdx < static_cast<int>(res.model.textures.size()) && res.model.textures[imgIdx].isValid()) {
						res.model.textures[imgIdx].bind();
					}
				}

				bgfx::setState(
					BGFX_STATE_WRITE_RGB		|
					BGFX_STATE_WRITE_A  		|
					BGFX_STATE_WRITE_Z			|
					BGFX_STATE_DEPTH_TEST_LESS	|
					BGFX_STATE_CULL_CCW
				);

				bgfx::submit(0, program);
			}
			mtxIdx++;
		}
	}
}
