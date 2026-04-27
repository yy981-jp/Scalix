#include "../model/model.h"
#include "cache.h"
#include "key.h"


void Avater::update(const uint64_t& keyStat) {
    // 向き操作
    if (has(keyStat, KCode::A)) yaw += 0.05f;
    if (has(keyStat, KCode::D)) yaw -= 0.05f;

    // 移動 → yawから直接XZ成分を計算してpos更新
    if (has(keyStat, KCode::W)) {
        pos[0] -= cachesv.getSin(yaw) * speed;
        pos[2] += cachesv.getCos(yaw) * speed;
    }
    if (has(keyStat, KCode::S)) {
        pos[0] += cachesv.getSin(yaw) * speed;
        pos[2] -= cachesv.getCos(yaw) * speed;
    }
    
}
