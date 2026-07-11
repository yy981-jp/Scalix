#pragma once

#include <vector>
#include <cstdint>
#include <limits>

struct Avatar;
struct Node;


using NodeId = int32_t; // -1(root)のためにsigned
using NodeEntryId = uint32_t; // NodeRegistry内のentryにを指すId
using NodeGen = uint16_t;

struct NodeHandle {
	NodeEntryId id;
	NodeGen gen;
};

class NodeRegistry {
    struct Entry {
        uint32_t nextFree; // 次の空きID（linked list）
        NodeGen gen = 0; // 世代

		Avatar* avatar = nullptr;
		NodeId node = INT32_MAX;
    };
    uint32_t aliveCount = 0;
    std::vector<Entry> records;

	uint32_t freeHead = UINT32_MAX; // フリーリストの先頭
    static constexpr uint32_t INVALID = std::numeric_limits<uint32_t>::max();

public:
    NodeHandle create();
    void destroy(NodeHandle h);
    bool is_alive(NodeHandle h) const;
    uint32_t size() const;

	Node& get(NodeHandle h) const;
};
