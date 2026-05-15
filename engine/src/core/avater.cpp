#include "avater.h"
#include "../anim/loader.h"
#include "../gltf/loader.h"
#include "cache.h"
#include "key.h"
#include "gctx.h"
#include "quatutil.h"


void Avater::update(GameContext& ctx, float dt) {
    // --- 移動 ---
    float fx = -lutsv.getSin(yaw);
    float fz =  lutsv.getCos(yaw);

    float rx =  fz;   // = cos(yaw)
    float rz = -fx;   // = sin(yaw)

    bool walking = false;

    float rlMove = 0.7;
    float aplSpeed = speed * dt;

    if (has(ctx.keyStat, KCode::W)) {
        walking = true;
        pos.x += fx * aplSpeed;
        pos.z += fz * aplSpeed;
    }
    if (has(ctx.keyStat, KCode::S)) {
        walking = true;
        pos.x -= fx * aplSpeed;
        pos.z -= fz * aplSpeed;
    }
    if (has(ctx.keyStat, KCode::A)) {
        walking = true;
        pos.x -= rx * aplSpeed * rlMove;
        pos.z -= rz * aplSpeed * rlMove;
    }
    if (has(ctx.keyStat, KCode::D)) {
        walking = true;
        pos.x += rx * aplSpeed * rlMove;
        pos.z += rz * aplSpeed * rlMove;
    }

    status = (walking? Status::walk : Status::stay);

    // --- 視点 ---
    if (has(ctx.keyStat, KCode::n0)) ctx.cam_type = CameraType::DEBUG;
    else if (has(ctx.keyStat, KCode::n1)) ctx.cam_type = CameraType::_1;

    head.yaw   += ctx.mStat.relPos.x * sensitivity;
    head.pitch -= ctx.mStat.relPos.y * sensitivity;

    // 制限
    if (head.pitch >  headPitchLimit) head.pitch =  headPitchLimit;
    if (head.pitch < -headPitchLimit) head.pitch = -headPitchLimit;

    if (head.yaw >  headYawLimit) {
        yaw -= head.yaw - headYawLimit; // 体を視点に追従
        head.yaw =  headYawLimit;
    }
    if (head.yaw < -headYawLimit) {
        yaw += -headYawLimit - head.yaw;
        head.yaw = -headYawLimit;
    }

    // --- neck (pitch) ---
    int neckIdx = humanoid.bones[(size_t)HBT::neck];
    auto& neckNode = model.nodes[neckIdx];
    neckNode.hasRotation = true;

    float qPitch[4];
    quatRotateAxis(qPitch, 1, 0, 0, head.pitch);

    for (int i = 0; i < 4; i++)
        neckNode.rot[i] = qPitch[i];

    // --- head (yaw) ---
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

    // headの向きから直接forward取得
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

Avater::Avater(const std::string& glTFPath) {
	model = loadGltf(glTFPath);
    humanoid.init(model.nodes, model.skins);
    anim.init("test.sxa");
}
