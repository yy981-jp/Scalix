#include "../model/model.h"
#include "cache.h"
#include "key.h"
#include "gctx.h"
#include "quatutil.h"


void Avater::update(GameContext& ctx) {
    // 体の向き（移動用）
    if (has(ctx.keyStat, KCode::A)) yaw += 0.05f;
    if (has(ctx.keyStat, KCode::D)) yaw -= 0.05f;

    if (has(ctx.keyStat, KCode::W)) {
        pos.x -= lutsv.getSin(yaw) * speed;
        pos.z += lutsv.getCos(yaw) * speed;
    }
    if (has(ctx.keyStat, KCode::S)) {
        pos.x += lutsv.getSin(yaw) * speed;
        pos.z -= lutsv.getCos(yaw) * speed;
    }

    // 視点変更
    if (has(ctx.keyStat, KCode::n0)) ctx.cam_type = CameraType::DEBUG;
    else if (has(ctx.keyStat, KCode::n1)) ctx.cam_type = CameraType::_1;

    head.yaw   += ctx.mStat.relPos.x * sensitivity;
    head.pitch -= ctx.mStat.relPos.y * sensitivity;

    // 制限（重要）
    if (head.pitch >  headPitchLimit) head.pitch =  headPitchLimit;
    if (head.pitch < -headPitchLimit) head.pitch = -headPitchLimit;

    if (head.yaw >  headYawLimit) head.yaw =  headYawLimit;
    if (head.yaw < -headYawLimit) head.yaw = -headYawLimit;

    // --- neck（pitch） ---
    int neckIdx = humanoid.bones[(size_t)HBT::neck];
    auto& neckNode = model.nodes[neckIdx];
    neckNode.hasRotation = true;

    float qPitch[4];
    quatRotateAxis(qPitch, 1, 0, 0, head.pitch);

    for (int i = 0; i < 4; i++)
        neckNode.rot[i] = qPitch[i];

    // --- head（yaw） ---
    int headIdx = humanoid.bones[(size_t)HBT::head];
    auto& headNode = model.nodes[headIdx];
    headNode.hasRotation = true;

    quatRotateAxis(headNode.rot, 0, 1, 0, head.yaw);

    // printf("hn.rot[1]: %g, [2]: %g, [3]: %g, [4]: %g\n", headNode.rot[1], headNode.rot[2], headNode.rot[3], headNode.rot[4]);
}


void Avater::draw(Camera& cam) {
    int headIdx = humanoid.bones[(size_t)HBT::head];

    float* m = globalMtxs[headIdx].data();

    vec3f headPos = {
        m[12],
        m[13],
        m[14]
    };

    // ★ headの向きから直接forward取得
    vec3f lookDir = {
        m[8],
        m[9],
        m[10]
    };

    lookDir = bx::normalize(lookDir);

    // モデルによっては逆向きなので必要なら反転
    // lookDir = -lookDir;

    vec3f camPos = headPos
        + lookDir * 0.1f
        + vec3f{0, 0.05f, 0};

    cam.update(camPos, camPos + lookDir);
}