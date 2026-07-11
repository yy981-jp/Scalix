#pragma once

#include <vector>
#include <cstdint>
#include <limits>


using NodeId = uint32_t;
using NodeGen = uint16_t;

struct NodeHandle {
	NodeId id;
	NodeGen gen;
};

class NodeManager {
    struct Entry {
        uint32_t nextFree; // 次の空きID（linked list）
        NodeGen gen = 0; // 世代
    };
    uint32_t aliveCount = 0;

public:
    NodeManager();
    NodeHandle create();
    void destroy(NodeHandle h);
    bool is_alive(NodeHandle h) const;
    uint32_t size() const;

private:
    static constexpr uint32_t INVALID = std::numeric_limits<uint32_t>::max();

    std::vector<Entry> records;
    uint32_t freeHead; // フリーリストの先頭
};
