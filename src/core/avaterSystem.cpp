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

        // 順序検証済み
        bx::mtxMul(
            avater.globalMtxs[idx].data(),
            localMtxs[idx].data(),
            avater.globalMtxs[node.parent].data()
        );
    
    } else {
        // ルートはentityから
        // calcGlobal(node.parent, calculated, avater, localMtxs, entityMtx);

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
        bx::mtxRotateX(rx, 0); // ← 座標系補正 (必要であれば-bx::kPiHalf ?)

        bx::mtxIdentity(s);
        s[0] = avater.scale[0];  s[5] = avater.scale[1];  s[10] = avater.scale[2];

        bx::mtxMul(tmp, s, rx);
        bx::mtxMul(tmp2, tmp, r);
        bx::mtxMul(entityMtx, tmp2, t);




        int nodeIdx = avater.humanoid.bones[
            static_cast<size_t>(HumanoidBoneType::arm_left_up)
        ];

        // int nodeIdx = avater.humanoid.spines[0];

        auto& node = avater.model.nodes[nodeIdx];

        // printf("bone idx: %d name: %s\n",
        //     nodeIdx,
        //     avater.model.nodes[nodeIdx].name.c_str()
        // );


        float addRot[4];
        quatRotateAxis(addRot, 1, 0, 0, 0.5f);

        quatMul(node.rot, addRot, node.rot);
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


void AvaterSystem::draw(bgfx::ProgramHandle program, bgfx::UniformHandle u_bones) {
    for (auto& avater : avaters) {

        // ===== とりあえず skin 0 を使う =====
        if (avater.model.skins.empty()) continue;
        auto& skin = avater.model.skins[0];

        // ===== joint行列作成 =====
        std::vector<std::array<float,16>> jointMtx;
        jointMtx.resize(skin.joints.size());

        for (int i = 0; i < (int)skin.joints.size(); i++) {
            int nodeIdx = skin.joints[i];

            bx::mtxMul(
                jointMtx[i].data(),
                skin.invBind[i].data(),
                avater.globalMtxs[nodeIdx].data()
            );
        }

        for (int nodeIdx = 0; nodeIdx < (int)avater.model.nodes.size(); nodeIdx++) {
            const auto& node = avater.model.nodes[nodeIdx];

            for (int i = 0; i < node.meshCount; i++) {
                const Mesh& m = avater.model.meshes[node.meshStartIndex + i];

                // ★ CPUスキニング用
                std::vector<Vertex> deformed;
                deformed.resize(m.verts.size());

                for (int vi = 0; vi < (int)m.verts.size(); vi++) {
                    const auto& v = m.verts[vi];
                    auto& out = deformed[vi];

                    float pos[4] = {v.x, v.y, v.z, 1.0f};
                    float result[4] = {0,0,0,0};
                    float weightSum = 0.0f;

                    // if (vi == 0) {
                    //     printf("w: %f %f %f %f\n",
                    //         v.weights[0], v.weights[1],
                    //         v.weights[2], v.weights[3]);
                    // }

                    for (int k = 0; k < 4; k++) {
                        float w = v.weights[k];
                        if (w <= 0.0f) continue;

                        int jointIdx = v.joints[k];
                        if (jointIdx < 0 || jointIdx >= (int)jointMtx.size()) continue;

                        float tmp[4];
                        bx::vec4MulMtx(tmp, pos, jointMtx[jointIdx].data());

                        result[0] += tmp[0] * w;
                        result[1] += tmp[1] * w;
                        result[2] += tmp[2] * w;
                        result[3] += tmp[3] * w;
                        weightSum += w;
                    }

                    out = v;
                    // Normalize by weight sum if needed
                    if (weightSum > 0.0f) {
                        out.x = result[0] / weightSum;
                        out.y = result[1] / weightSum;
                        out.z = result[2] / weightSum;
                    } else {
                        out.x = v.x;
                        out.y = v.y;
                        out.z = v.z;
                    }
                }


                bgfx::VertexLayout layout;
                Vertex::init(layout);

                // ===== 毎フレームVB生成（最小構成） =====
                auto vbh = bgfx::createVertexBuffer(
                    bgfx::copy(deformed.data(), sizeof(Vertex) * deformed.size()),
                    layout
                );

                // ★ transformはidentityにする（スキニング済みなので）
                float identity[16];
                bx::mtxIdentity(identity);
                bgfx::setTransform(identity);

                bgfx::setVertexBuffer(0, vbh);
                bgfx::setIndexBuffer(m.ibh);

                // テクスチャ
                if (m.materialIndex >= 0 && m.materialIndex < (int)avater.model.materialToImage.size()) {
                    int imgIdx = avater.model.materialToImage[m.materialIndex];
                    if (imgIdx >= 0 && imgIdx < (int)avater.model.textures.size() && avater.model.textures[imgIdx].isValid()) {
                        avater.model.textures[imgIdx].bind();
                    }
                }

                bgfx::setState(
                    BGFX_STATE_WRITE_RGB        |
                    BGFX_STATE_WRITE_A          |
                    BGFX_STATE_WRITE_Z            |
                    BGFX_STATE_DEPTH_TEST_LESS    |
                    BGFX_STATE_CULL_CCW
                );

                bgfx::submit(0, program);

                bgfx::destroy(vbh);
            }
        }
    }
}


/*
void AvaterSystem::draw(bgfx::ProgramHandle program, bgfx::UniformHandle u_bones) {
    for (auto& avater : avaters) {

        // 骨構造を持たないavaterは処理しない
        if (avater.model.skins.empty()) continue;


        std::vector<std::vector<std::array<float,16>>> allJointMtx;
        allJointMtx.resize(avater.model.skins.size());

        for (int skinIdx = 0; skinIdx < avater.model.skins.size(); skinIdx++) {
            const Skin& skin = avater.model.skins[skinIdx];

            auto& jointMtx = allJointMtx[skinIdx];
            jointMtx.resize(skin.joints.size());

            for (int i = 0; i < (int)skin.joints.size(); i++) {
                int nodeIdx = skin.joints[i];

                // 順序検証済み
                bx::mtxMul(
                    jointMtx[i].data(),
                    skin.invBind[i].data(),
                    avater.globalMtxs[nodeIdx].data()
                );
            }
            for (int i = 0; i < 10; i++) {
                int nodeIdx = skin.joints[i];

                printf("joint %d -> node %d\n", i, nodeIdx);

                printf("global pos: %f %f %f\n",
                    avater.globalMtxs[nodeIdx][12],
                    avater.globalMtxs[nodeIdx][13],
                    avater.globalMtxs[nodeIdx][14]
                );
            }
        }


        // === nodeのloop 描画loop本体とも言える ===
        for (int nodeId = 0; nodeId < avater.model.nodes.size(); nodeId++) {
            const Node& node = avater.model.nodes[nodeId];

            // 処理する必要のないものを除外
            if (node.meshCount == 0) continue;
            if (node.skinIndex < 0) continue;

            for (int i = 0; i < node.meshCount; i++) {
                const Mesh& mesh = avater.model.meshes[node.meshStartIndex + i];

                // ===== パレット =====
                std::vector<std::array<float,16>> palette;
                palette.resize(mesh.boneRemapInverse.size());

                for (int i = 0; i < (int)mesh.boneRemapInverse.size(); i++) {
                    int orig = mesh.boneRemapInverse[i];
                    palette[i] = allJointMtx[node.skinIndex][orig];
                }

                // for (int i = 0; i < (int)mesh.boneRemapInverse.size(); i++) {
                //     int jointIdx = mesh.boneRemapInverse[i];

                //     int nodeIdx =  avater.model.skins[node.skinIndex].joints[jointIdx];

                //     printf("palette[%d] -> joint %d -> node %d\n", i, jointIdx, nodeIdx);
                // }

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
                    mesh.materialIndex < (int)avater.model.materialToImage.size()) {

                    int imgIdx = avater.model.materialToImage[mesh.materialIndex];

                    if (imgIdx >= 0 &&
                        imgIdx < (int)avater.model.textures.size() &&
                        avater.model.textures[imgIdx].isValid()) {

                        avater.model.textures[imgIdx].bind();
                    }
                }

                // ===== state =====
                bgfx::setState(
                    BGFX_STATE_WRITE_RGB |
                    BGFX_STATE_WRITE_A   |
                    BGFX_STATE_WRITE_Z   |
                    BGFX_STATE_DEPTH_TEST_LESS |
                    BGFX_STATE_CULL_CCW
                );

                // ===== draw =====
                bgfx::submit(0, program);
            }

            
        }
    }
}
*/
