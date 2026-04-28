#include "../model/model.h"
#include "cache.h"
#include "key.h"
#include "gctx.h"


void Avater::update(GameContext& ctx) {
    // 向き操作
    if (has(ctx.keyStat, KCode::A)) yaw += 0.05f;
    if (has(ctx.keyStat, KCode::D)) yaw -= 0.05f;

    // 移動 → yawから直接XZ成分を計算してpos更新
    if (has(ctx.keyStat, KCode::W)) {
        pos.x -= cachesv.getSin(yaw) * speed;
        pos.z += cachesv.getCos(yaw) * speed;
    }
    if (has(ctx.keyStat, KCode::S)) {
        pos.x += cachesv.getSin(yaw) * speed;
        pos.z -= cachesv.getCos(yaw) * speed;
    }

    if (has(ctx.keyStat, KCode::K)) c_u -= 0.1;
    if (has(ctx.keyStat, KCode::I)) c_u += 0.1;
    
}

void Avater::draw(Camera& cam) {
    // camera 更新
    int headIdx = humanoid.bones[(size_t)HBT::head];

    float* m = globalMtxs[headIdx].data();

    vec3f headWorld = {
        m[12],
        m[13],
        m[14]
    };
    cam.update(headWorld,{0,c_u,1});
}