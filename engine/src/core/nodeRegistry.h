#pragma once

#include <def/node.h>

#include <vector>
#include <cstdint>
#include <limits>

struct Avatar;
using AvatarId = int;
struct Node;


class NodeRegistry {
	struct Entry {
		uint32_t nextFree; // 次の空きID（linked list）
		NodeGen gen = 0; // 世代

		Avatar* avatar = nullptr;
		AvatarId avatarId;
		NodeId node = UINT32_MAX;
	};
	uint32_t aliveCount = 0;
	std::vector<Entry> records;

	uint32_t freeHead = UINT32_MAX; // フリーリストの先頭
	static constexpr uint32_t INVALID = std::numeric_limits<uint32_t>::max();


	NodeHandle find(Entry entry, NodeId nodeId) const;

public:
	NodeHandle create(Avatar* avatar, NodeId id);
	void destroy(NodeHandle h);
	bool is_alive(NodeHandle h) const;
	uint32_t size() const;

	Node& get(NodeHandle h);
	NodeId getId(NodeHandle h);
	Avatar* getAvatar(NodeHandle h);
	
	const Entry& getEntry(NodeHandle h) const;
	NodeHandle find(NodeHandle avaRefHandle, NodeId nodeId) const;

	// ほぼデバッグ用
	// 計算コスト的に過度な呼び出しは注意
	NodeHandle findFromAvaId(AvatarId avatarId, NodeId nodeId) const;

};

extern NodeRegistry nodeReg;
