#include "../model/model.h"
#include "cache.h"
#include "key.h"
#include "gctx.h"
#include "quatutil.h"


void Avater::update(GameContext& ctx) {
    // 向き操作
    if (has(ctx.keyStat, KCode::A)) yaw += 0.05f;
    if (has(ctx.keyStat, KCode::D)) yaw -= 0.05f;

    // 移動 → yawから直接XZ成分を計算してpos更新
    if (has(ctx.keyStat, KCode::W)) {
        pos.x -= lutsv.getSin(yaw) * speed;
        pos.z += lutsv.getCos(yaw) * speed;
    }
    if (has(ctx.keyStat, KCode::S)) {
        pos.x += lutsv.getSin(yaw) * speed;
        pos.z -= lutsv.getCos(yaw) * speed;
    }

    if (has(ctx.keyStat, KCode::K)) c_u -= 0.1;
    if (has(ctx.keyStat, KCode::I)) c_u += 0.1;

    
	int headIdx = humanoid.bones[(size_t)HBT::head];
    auto& headNode = model.nodes[headIdx];
    headNode.hasRotation = true;

    float sensitivity = 0.01;

    head.yaw   -= ctx.mStat.relPos.x * sensitivity;
    head.pitch -= ctx.mStat.relPos.y * sensitivity;

	// printf("D:   abs.x: %d,  abs.y: %d,  rel.x: %d,  rel.y: %d\n",
    //     ctx.mStat.absPos.x, ctx.mStat.absPos.y, ctx.mStat.relPos.x, ctx.mStat.relPos.y);


    lookDir.x = lutsv.getCos(head.yaw) * lutsv.getCos(head.pitch);
    lookDir.y = lutsv.getSin(head.pitch);
    lookDir.z = lutsv.getSin(head.yaw) * lutsv.getCos(head.pitch);

    lookDir = bx::normalize(lookDir);

    float qYaw[4];
    float qPitch[4];

    // yawはY軸
    quatRotateAxis(qYaw, 0, 1, 0, head.yaw);

    // pitchはX軸
    quatRotateAxis(qPitch, 1, 0, 0, head.pitch);

    // 合成（順序重要）
    float rot[4];
    quatMul(rot, qYaw, qPitch);

    for (int i = 0; i < 4; i++)
        headNode.rot[i] = rot[i];

}

void Avater::draw(Camera& cam) {
	int headIdx = humanoid.bones[(size_t)HBT::head];

	float* m = globalMtxs[headIdx].data();

	vec3f headPos = {
		m[12],
		m[13],
		m[14]
	};

    // 少し前＆ちょい上
	vec3f camPos = headPos
		+ lookDir * 0.1f
		+ vec3f{0, 0.05f, 0};

	cam.update(camPos, camPos + lookDir);
    // printf("D: lookDir: x:%g, y:%g, z:%g\n",
    //     lookDir.x, lookDir.y, lookDir.z );
}
