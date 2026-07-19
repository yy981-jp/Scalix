#include <core/nodeRegistry.h>
#include <core/avatar.h>

NodeHandle NodeRegistry::create(Avatar* avatar, NodeId nodeid) {
	++aliveCount;
	NodeEntryId id;

	if (freeHead != INVALID) {
		// フリースロットを再利用
		id = freeHead;
		freeHead = records[id].nextFree;
		records[id].avatarId = avatar->id;
		records[id].avatar = avatar;
		records[id].node = nodeid;
	} else {
		// 新規スロット追加
		id = (NodeEntryId)records.size();
		records.push_back({ INVALID, 0, avatar, avatar->id, nodeid });
	}

	return {id,records[id].gen};
}

void NodeRegistry::destroy(NodeHandle h) {
	--aliveCount;

	++records[h.id].gen;
	records[h.id].avatar = nullptr;
	records[h.id].node = INT32_MAX;

	// フリースロット linked list に追加
	records[h.id].nextFree = freeHead;
	freeHead = h.id;
}

bool NodeRegistry::is_alive(NodeHandle h) const {
	return h.id < records.size() && records[h.id].avatar != nullptr && records[h.id].gen == h.gen;
}

uint32_t NodeRegistry::size() const {
	return aliveCount;
}

Node& NodeRegistry::get(NodeHandle h) {
	if (!is_alive(h)) throw std::runtime_error("NodeRegistry::get(): is not allive");

	Entry& entry = records[h.id];
	Node& node = entry.avatar->model.nodes[entry.node];
	
	return node;
}

NodeId NodeRegistry::getId(NodeHandle h) {
	if (!is_alive(h)) throw std::runtime_error("NodeRegistry::getId():: is no allive");

	Entry& entry = records[h.id];
	return entry.node;
}

Avatar* NodeRegistry::getAvatar(NodeHandle h) {
	if (!is_alive(h)) throw std::runtime_error("NodeRegistry::getId():: is no allive");

	Entry& entry = records[h.id];
	return entry.avatar;
}

const NodeRegistry::Entry& NodeRegistry::getEntry(NodeHandle h) const {
	if (!is_alive(h)) throw std::runtime_error("NodeRegistry::getId():: is no allive");
	return records[h.id];
}


NodeHandle NodeRegistry::nd_find(NodeId id) const {
	for (uint32_t i = 0; i < records.size(); ++i) {
		NodeHandle h{i, records[i].gen};

		if (!is_alive(h)) continue;

		if (h.id == id) return h;
	}
	return {};
}

NodeHandle NodeRegistry::find(AvatarId avatarId, NodeId nodeId) const {
	for (uint32_t i = 0; i < records.size(); ++i) {
		NodeHandle h{i, records[i].gen};

		if (!is_alive(h))
			continue;

		const Entry& e = records[i];
		if (e.avatarId == avatarId && e.node == nodeId)
			return h;
	}

	return {};
}



NodeRegistry nodeReg;
