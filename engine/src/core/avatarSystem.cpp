#include "avatarSystem.h"

#include "../gltf/loader.h"
#include "key.h"
#include "mtxutil.h"
#include "cache.h"
#include "quatutil.h"

#include <bx/math.h>
#include <iostream>


void AvatarSystem::loadData(const std::vector<std::string> path) {
    AvatarID id = 0;
    for (const auto& file: path) {
        Avatar avatar(file);
        avatar.id = id++;
		avatars.push_back(std::move(avatar));
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
    
    for (auto& avatar : avatars) {

        // printf("D: avatar.id: %d, playable: %d\n", avatar.id, playableAvatar);
        if (avatar.id != playableAvatar) continue; // player以外の制御はしない

        // avatar update
        avatar.update(ctx,dt);

        avatar.globalMtxs.clear();
        avatar.globalMtxs.resize(avatar.model.nodes.size());

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


void AvatarSystem::draw(bgfx::ProgramHandle program) {
    for (auto& avatar : avatars) {

        // ===== とりあえず skin 0 を使う =====
        if (avatar.model.skins.empty()) continue;
        auto& skin = avatar.model.skins[0];

        // ===== joint行列作成 =====
        std::vector<std::array<float,16>> jointMtx;
        jointMtx.resize(skin.joints.size());

        for (int i = 0; i < (int)skin.joints.size(); i++) {
            NodeId nodeIdx = skin.joints[i];

            bx::mtxMul(
                jointMtx[i].data(),
                skin.invBind[i].data(),
                avatar.globalMtxs[nodeIdx].data()
            );
        }

        for (NodeId nodeIdx = 0; nodeIdx < (int)avatar.model.nodes.size(); nodeIdx++) {
            const auto& node = avatar.model.nodes[nodeIdx];

            if (!node.visible) continue;

            for (int i = 0; i < node.meshCount; i++) {
                const Mesh& m = avatar.model.meshes[node.meshStartIndex + i];

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
                if (m.materialIndex >= 0 && m.materialIndex < (int)avatar.model.materialToImage.size()) {
                    int imgIdx = avatar.model.materialToImage[m.materialIndex];
                    if (imgIdx >= 0 && imgIdx < (int)avatar.model.textures.size() && avatar.model.textures[imgIdx].isValid()) {
                        avatar.model.textures[imgIdx].bind();
                    }
                }

                bgfx::setState(
                    BGFX_STATE_WRITE_RGB        |
                    BGFX_STATE_WRITE_A          |
                    BGFX_STATE_WRITE_Z            |
                    BGFX_STATE_DEPTH_TEST_LESS    |
                    BGFX_STATE_CULL_CW
                );

                bgfx::submit(0, program);

                bgfx::destroy(vbh);
            }
        }
    }
}


/*
void AvatarSystem::draw(bgfx::ProgramHandle program, bgfx::UniformHandle u_bones) {
    for (auto& avatar : avatars) {

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
            for (int i = 0; i < 10; i++) {
                NodeId nodeIdx = skin.joints[i];

                printf("joint %d -> node %d\n", i, nodeIdx);

                printf("global pos: %f %f %f\n",
                    avatar.globalMtxs[nodeIdx][12],
                    avatar.globalMtxs[nodeIdx][13],
                    avatar.globalMtxs[nodeIdx][14]
                );
            }
        }


        // === nodeのloop 描画loop本体とも言える ===
        for (NodeId nodeId = 0; nodeId < avatar.model.nodes.size(); nodeId++) {
            const Node& node = avatar.model.nodes[nodeId];

            // 処理する必要のないものを除外
            if (node.meshCount == 0) continue;
            if (node.skinIndex < 0) continue;

            for (int i = 0; i < node.meshCount; i++) {
                const Mesh& mesh = avatar.model.meshes[node.meshStartIndex + i];

                // ===== パレット =====
                std::vector<std::array<float,16>> palette;
                palette.resize(mesh.boneRemapInverse.size());

                for (int i = 0; i < (int)mesh.boneRemapInverse.size(); i++) {
                    int orig = mesh.boneRemapInverse[i];
                    palette[i] = allJointMtx[node.skinIndex][orig];
                }

                // for (int i = 0; i < (int)mesh.boneRemapInverse.size(); i++) {
                //     int jointIdx = mesh.boneRemapInverse[i];

                //     int nodeIdx =  avatar.model.skins[node.skinIndex].joints[jointIdx];

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
                    BGFX_STATE_CULL_CCW
                );

                // ===== draw =====
                bgfx::submit(0, program);
            }

            
        }
    }
}
*/
