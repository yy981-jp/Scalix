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
	int headIdx = humanoid.bones[(size_t)HBT::head];

	float* m = globalMtxs[headIdx].data();

	vec3f headPos = {
		m[12],
		m[13],
		m[14]
	};

	// 行列からforward取り出し
	vec3f forward = {
		m[8],
		m[9],
		m[10]
	};

    //tmp
    forward.y += c_u;

    forward = bx::normalize(forward);


    // 少し前＆ちょい上
	vec3f camPos = headPos
		+ forward * 0.1f
		+ vec3f{0, 0.05f, 0};

	cam.update(camPos, forward);
}
