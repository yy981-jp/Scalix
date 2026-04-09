#include "entityManager.h"

EntityManager::EntityManager() {
    freeHead = INVALID;
}

EntityHandle EntityManager::create() {
    ++aliveCount;
    EntityID id;

    if (freeHead != INVALID) {
        // フリースロットを再利用
        id = freeHead;
        freeHead = records[id].nextFree;
    } else {
        // 新規スロット追加
        id = (EntityID)records.size();
        records.push_back({INVALID});
    }

    return {id,records[id].gen};
}

void EntityManager::destroy(EntityHandle h) {
    --aliveCount;

    ++records[h.id].gen;

    // フリースロット linked list に追加
    records[h.id].nextFree = freeHead;
    freeHead = h.id;
}

bool EntityManager::is_alive(EntityHandle h) const {
    return records[h.id].gen == h.gen;
}

uint32_t EntityManager::size() const {
    return aliveCount;
}


EntityManager entMgr;
