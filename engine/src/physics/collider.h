#pragma once
#include <cstdint>
#include <vector>


/*
class ColliderSystem {
public:
    struct Entry {
        uint32_t nextFree;
        uint32_t gen;
    };

private:
    static constexpr ColliderID INVALID = UINT32_MAX;

    // ----- SoA -----
    // 共通
    std::vector<ColliderType> type;
    std::vector<vec2f> pos;        // center (circle) / center (rect)
    std::vector<CollisionLayer> layer;
    std::vector<uint8_t> mask;

    // Circle専用
    std::vector<float> radius;

    // Rect専用
    std::vector<vec2f> rectMin, rectMax;

    // AABB (broadcheck用 大雑把な当たり判定)
    std::vector<vec2f> aabbMin, aabbMax;
    
    std::vector<EntityHandle> owner;

    std::vector<ColliderID> alive;
    std::vector<uint32_t> aliveIndex;
    std::vector<Entry> records;
    ColliderID freeHead = INVALID;

    // ptr
    std::vector<ICollidable*> ptr_collidable;
    std::vector<ColPtrSet> ptr_set;

    // ----- Func -----
    bool hitCircleCircle(EntityID a, EntityID b);
    bool hitRectCircle(EntityID r, EntityID c);
    bool hitCircleRect(EntityID c, EntityID r);
    bool hitRectRect(EntityID a, EntityID b);
    using HitFunc = bool (ColliderSystem::*)(EntityID, EntityID);
    HitFunc hitTable[2][2] = {
        { &ColliderSystem::hitCircleCircle, &ColliderSystem::hitCircleRect },
        { &ColliderSystem::hitRectCircle,   &ColliderSystem::hitRectRect }
    };

    void updateAABB(EntityID id);
    bool aabbOverlap(EntityID a, EntityID b);
    bool shouldCollide(EntityID a, EntityID b);
    HitEvent genHitInfo(EntityID a, EntityID b);

public:
    ICollidable* getColPtr(const ColliderHandle& h) const {
        if (!isAlive(h)) return {};
        return ptr_collidable[h.id];
    }

    ColPtrSet getPtr(const ColliderHandle& h) const {
        if (!isAlive(h)) return {};
        return ptr_set[h.id];
    }

    void draw(const Renderer* r);

    ColliderHandle add(const Collider& c);

    void setColPtr(ColliderHandle h, ICollidable* ptr) {
        if (!isAlive(h)) return;
        ptr_collidable[h.id] = ptr;
    }

    void setPtr(ColliderHandle h, const ColPtrSet& ptrSet) {
        if (!isAlive(h)) return;
        ptr_set[h.id] = ptrSet;
    }

    // 判定ループ（circle only broadphase）
    void step(GCMS& gcm);

    void setPos(ColliderHandle h, vec2f p) {
        pos[h.id] = p;
    }

    void destroy(ColliderHandle h);
    bool isAlive(ColliderHandle h) const;
};


struct Contact {
	ColliderHandle a;
	ColliderHandle b;

	vec3f point;
	vec3f normal;
	float penetration;
};


class ColliderSystem
*/