#pragma once

#include <def/node.h>
#include <def/mtx.h>
#include <model/avatarDef.h>

#include <vector>
#include <cstdint>
#include <limits>

struct Node;


class NodeRegistry {
	struct Entry {
		uint32_t nextFree; // 次の空きID（linked list）
		NodeGen gen = 0; // 世代

		// AvatarSystem::avatarsがunordered_mapだからできる
		Avatar* avatar = nullptr;
		AvatarId avatarId;
		
		NodeId node = UINT32_MAX;
	};
	uint32_t aliveCount = 0;
	std::vector<Entry> records;

	uint32_t freeHead = UINT32_MAX; // フリーリストの先頭
	static constexpr uint32_t INVALID = std::numeric_limits<uint32_t>::max();


public:
	NodeHandle create(Avatar* avatar, NodeId id);
	void destroy(NodeHandle h);

	uint32_t size() const;

	bool isAlive(NodeHandle h) const;
	Entry& getEntry(NodeHandle h);

	Node& get(NodeHandle h);
	NodeId getId(NodeHandle h);
	Avatar* getAvatar(NodeHandle h);
	Mtx& getGlobalMtx(NodeHandle h);
};

extern NodeRegistry nodeReg;
