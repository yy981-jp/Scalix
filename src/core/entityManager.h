#pragma once

#include <vector>
#include <cstdint>
#include <limits>


using EntityGen = uint16_t;    // 世代
using EntityID = uint32_t;      // 個体別
struct EntityHandle {
    EntityID id;
    EntityGen gen;
};


class EntityManager {
    struct Entry {
        uint32_t nextFree; // 次の空きID（linked list）
        EntityGen gen = 0; // 世代
    };
    uint32_t aliveCount = 0;

public:
    EntityManager();
    EntityHandle create();
    void destroy(EntityHandle h);
    bool is_alive(EntityHandle h) const;
    uint32_t size() const;

private:
    static constexpr uint32_t INVALID = std::numeric_limits<uint32_t>::max();

    std::vector<Entry> records;
    uint32_t freeHead; // フリーリストの先頭
};


extern EntityManager entMgr;
