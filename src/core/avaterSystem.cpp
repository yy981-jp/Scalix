#include "avaterSystem.h"

#include "../gltf/loader.h"
#include "key.h"
#include "mtxutil.h"
#include "cache.h"
#include "quatutil.h"

#include <bx/math.h>
#include <iostream>


void AvaterSystem::loadData(const std::vector<std::string> path) {
	for (const auto& file: path) {
		avaters.push_back( loadEntity(file) );
        auto& avater = avaters.back();
        avater.humanoid.init(avater.model.nodes, avater.model.skins);
    }
}

void calcGlobal(int idx, std::vector<bool>& calculated, Avater& avater,
                std::vector<std::array<float,16>>& localMtxs, float* entityMtx) {
    if (calculated[idx]) return;
    if (idx < 0 || idx >= avater.model.nodes.size()) {
        printf("invalid idx: %d\n", idx);
        return;
    }

    const auto& node = avater.model.nodes[idx];


    // 親が先
    if (node.parent >= 0) {
        calcGlobal(node.parent, calculated, avater, localMtxs, entityMtx);

        bx::mtxMul(
            avater.globalMtxs[idx].data(),
            localMtxs[idx].data(),      // ← child local が先
            avater.globalMtxs[node.parent].data()  // ← parent が後
        );
    
    } else {
        // ルートはentityから

        bx::mtxMul(
            avater.globalMtxs[idx].data(),
            entityMtx,
            localMtxs[idx].data()
        );
    }

    calculated[idx] = true;
}


void AvaterSystem::update(const uint64_t& keyStat) {
    for (auto& avater : avaters) {
        avater.globalMtxs.clear();
        avater.globalMtxs.resize(avater.model.nodes.size());

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
        float t[16], r[16], s[16], rx[16], tmp[16], tmp2[16], entityMtx[16];

        bx::mtxTranslate(t, avater.pos[0], avater.pos[1], avater.pos[2]);
        bx::mtxRotateY(r, avater.yaw);
        bx::mtxRotateX(rx, -bx::kPiHalf); // ← 座標系補正

        bx::mtxIdentity(s);
        s[0] = avater.scale[0];  s[5] = avater.scale[1];  s[10] = avater.scale[2];

        bx::mtxMul(tmp, s, rx);
        bx::mtxMul(tmp2, tmp, r);
        bx::mtxMul(entityMtx, tmp2, t);




        int nodeIdx = avater.humanoid.bones[
            static_cast<size_t>(HumanoidBoneType::arm_left_low)
        ];

        auto& node = avater.model.nodes[nodeIdx];

        // printf("bone idx: %d name: %s\n",
        //     nodeIdx,
        //     avater.model.nodes[nodeIdx].name.c_str()
        // );

        float addRot[4];
        quatRotateAxis(addRot, 1, 0, 0, 1);

        quatMul(node.rot, node.rot, addRot);
        quatNormalize(node.rot);




        // local mtx
        std::vector<std::array<float, 16>> localMtxs;
        localMtxs.resize(avater.model.nodes.size());

        for (int i = 0; i < avater.model.nodes.size(); i++) {
            const auto& node = avater.model.nodes[i];

            buildTRS(
                localMtxs[i].data(),
                node.pos,
                node.rot,
                node.scale,
                node.hasRotation
            );
        }

        std::vector<bool> calculated;
        calculated.resize(avater.model.nodes.size(), false);

        for (int i = 0; i < avater.model.nodes.size(); i++) {
            calcGlobal(i,calculated,avater,localMtxs,entityMtx);
        }

    }
}

void AvaterSystem::draw(bgfx::ProgramHandle program) {
    for (auto& avater : avaters) {
        for (int nodeIdx = 0; nodeIdx < (int)avater.model.nodes.size(); nodeIdx++) {
            const auto& node = avater.model.nodes[nodeIdx];

            // if (node.meshCount <= 0) continue; // メッシュがないなら行列も使わないのでスキップ

            for (int i = 0; i < node.meshCount; i++) {
                const Mesh& m = avater.model.meshes[node.meshStartIndex + i];

                // ★ nodeIdx を直接使うことで、updateで計算した箇所と一致させる
                bgfx::setTransform(avater.globalMtxs[nodeIdx].data());

                bgfx::setVertexBuffer(0, m.vbh);
                bgfx::setIndexBuffer(m.ibh);
                
				// テクスチャバインド（マテリアル → イメージ → テクスチャ）
				if (m.materialIndex >= 0 && m.materialIndex < static_cast<int>(avater.model.materialToImage.size())) {
					int imgIdx = avater.model.materialToImage[m.materialIndex];
					if (imgIdx >= 0 && imgIdx < static_cast<int>(avater.model.textures.size()) && avater.model.textures[imgIdx].isValid()) {
						avater.model.textures[imgIdx].bind();
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
        }
    }
}