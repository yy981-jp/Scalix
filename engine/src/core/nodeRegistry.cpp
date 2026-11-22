#include <core/nodeRegistry.h>
#include <core/avatar.h>

NodeHandle NodeRegistry::create(Avatar* avatar, NodeId nodeid) {
	++aliveCount;
	NodeEntryId id;
	
	assert(avatar);

	if (freeHead != INVALID) {
		// フリースロットを再利用
		id = freeHead;
		freeHead = records[id].nextFree;
		records[id].avatar = avatar;
		records[id].avatarId = avatar->id;
		records[id].node = nodeid;
	} else {
		// 新規スロット追加
		id = (NodeEntryId)records.size();
		records.push_back({
			.nextFree = INVALID,
			.gen = 0,
			.avatar = avatar,
			.avatarId = avatar->id,
			.node = nodeid
		});
	}

	return {id,records[id].gen};
}

void NodeRegistry::destroy(NodeHandle h) {
	--aliveCount;

	++records[h.id].gen;
	records[h.id].avatar = nullptr;
	records[h.id].avatarId = {};
	records[h.id].node = NodeId::invalid();

	// フリースロット linked list に追加
	records[h.id].nextFree = freeHead;
	freeHead = h.id;
}

uint32_t NodeRegistry::size() const {
	return aliveCount;
}

bool NodeRegistry::isAlive(NodeHandle h) const {
	return h.isValid() && h.id < records.size() && records[h.id].gen == h.gen;
}

NodeRegistry::Entry& NodeRegistry::getEntry(NodeHandle h) {
	if (!isAlive(h)) throw std::runtime_error("NodeRegistry::getId():: is not allive");
	return records[h.id];
}

Node& NodeRegistry::get(NodeHandle h) {
	Entry& entry = getEntry(h);
	Node& node = entry.avatar->model.nodes[entry.node];
	
	return node;
}

NodeId NodeRegistry::getId(NodeHandle h) {
	return getEntry(h).node;
}

Avatar* NodeRegistry::getAvatar(NodeHandle h) {
	return getEntry(h).avatar;
}

Mtx& NodeRegistry::getGlobalMtx(NodeHandle h) {
	auto& entry = getEntry(h);
	return entry.avatar->globalMtx[entry.node];
}


NodeRegistry nodeReg;
