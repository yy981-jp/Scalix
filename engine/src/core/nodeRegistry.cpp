#include <core/nodeRegistry.h>
#include <core/avatar.h>

NodeHandle NodeRegistry::create() {
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

void NodeRegistry::destroy(NodeHandle h) {
    --aliveCount;

    ++records[h.id].gen;

    // フリースロット linked list に追加
    records[h.id].nextFree = freeHead;
    freeHead = h.id;
}

bool NodeRegistry::is_alive(NodeHandle h) const {
    return records[h.id].gen == h.gen;
}

uint32_t NodeRegistry::size() const {
    return aliveCount;
}

Node& NodeRegistry::get(NodeHandle h) const {
	if (!is_alive(h)) throw std::runtime_error("NodeRegistry::get(): is not allive");

	Entry& entry = records[h.id];
	Node& node = entry.avatar->model.nodes[entry.node];
	
	return node;
}
