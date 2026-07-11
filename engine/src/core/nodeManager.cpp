#include <core/nodeManager.h>

NodeManager::NodeManager() {
    freeHead = INVALID;
}

NodeHandle NodeManager::create() {
    ++aliveCount;
    NodeId id;

    if (freeHead != INVALID) {
        // フリースロットを再利用
        id = freeHead;
        freeHead = records[id].nextFree;
    } else {
        // 新規スロット追加
        id = (NodeId)records.size();
        records.push_back({INVALID});
    }

    return {id,records[id].gen};
}

void NodeManager::destroy(NodeHandle h) {
    --aliveCount;

    ++records[h.id].gen;

    // フリースロット linked list に追加
    records[h.id].nextFree = freeHead;
    freeHead = h.id;
}

bool NodeManager::is_alive(NodeHandle h) const {
    return records[h.id].gen == h.gen;
}

uint32_t NodeManager::size() const {
    return aliveCount;
}
