#pragma once
#include <cstdint>
#include <vector>

#include <def/id.h>
#include <def/vec3.h>


class Collidertag {};
using ColliderId = Id<Collidertag>;
using ColliderGen = uint32_t;
struct ColliderHandle {
	ColliderId id;
	ColliderGen gen;
};


struct Collider {
	
};


class ColliderSystem {
	struct Entry {
		ColliderId nextFree;
		uint32_t gen;
	};

	
	// ↓ 頭が混乱しそうなので詳しく書いとく

	// 有効なColliderのId
	std::vector<ColliderId> alive;
	// aliveの順序をColiderIdから追跡するため
	std::vector<ColliderId> aliveIndex;
	// 各スロットの世代を保持し、空きの連結リストにも使う
	std::vector<Entry> records;
	// 空きスロットの連結リストの終端
	ColliderId freeHead = ColliderId::invalid();

	// 再利用可能なデータ領域
	std::vector<Collider> colliders;


public:

	ColliderHandle add(const Collider& collider) {
		ColliderId id;
		if (freeHead != ColliderId::invalid()) {
			// 再利用
			id = freeHead;
			freeHead = records[id].nextFree;
		} else {
			// 新規確保
			id = records.size();
			records.emplace_back();
			aliveIndex.resize(records.size());
		}
		aliveIndex[id] = alive.size();
		alive.push_back(id);

		colliders[id] = collider;


		ColliderGen gen = records[id].gen;
		return {
			.id = id,
			.gen = gen
		};
	};


	void destroy(const ColliderHandle& hdl) {
		if (!isAlive(hdl)) return;

		const ColliderId& idx  = aliveIndex[hdl.id];
		const ColliderId& last = alive.back();

		// 効率的に削除
		alive[idx] = last;
		aliveIndex[last] = idx;
		alive.pop_back();

		// 再利用できるようにするための処理
		records[hdl.id].gen++;
		records[hdl.id].nextFree = freeHead;
		freeHead = hdl.id;
	}


	bool isAlive(const ColliderHandle& hdl) {
		
	}

	
	void tick() {
	};



};


struct Contact {
	ColliderHandle a;
	ColliderHandle b;

	vec3f point;
	vec3f normal;
	float penetration;
};
