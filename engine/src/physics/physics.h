#pragma once

#include <def/vec3.h>




struct Physics {
	int avatarId;
	NodeId node;
};

using PhysicsId = uint32_t;
using PhysicsGen = uint16_t;

struct PhysicsHandle {
	PhysicsId id;
	PhysicsGen gen;
};


class PhysicsSystem {
public:
	struct Entry {
		uint32_t nextFree;
		PhysicsGen gen;
	};

	struct Data {

	};

	
private:
	std::vector<Physics> phys;

	PhysicsId INVALID = UINT32_MAX;

	std::vector<PhysicsId> alive;
	std::vector<uint32_t> aliveIndex;
	std::vector<Entry> records;
	PhysicsId freeHead = INVALID;

	std::vector<Data> data;

public:
	PhysicsHandle add(const Physics& c) {
	    PhysicsId id;
		if (freeHead != INVALID) {
			id = freeHead;
	        freeHead = records[id].nextFree;
		} else {
			id = static_cast<PhysicsId>(records.size());
	        records.push_back({ INVALID, 0 });
			data.push_back({});
		}

		// ===== alive 登録（共通）=====
		if (aliveIndex.size() < records.size())
			aliveIndex.resize(records.size());

		aliveIndex[id] = alive.size();
		alive.push_back(id);

		PhysicsGen gen = records[id].gen;

	    return { id, gen };
	}

	bool isAlive(PhysicsHandle h) const {
		return h.id < records.size()
			&& records[h.id].gen == h.gen;
	}

	void destroy(PhysicsHandle h) {
		if (!isAlive(h)) return;

		uint32_t idx  = aliveIndex[h.id];
		PhysicsId last = alive.back();

		alive[idx] = last;
		aliveIndex[last] = idx;
		alive.pop_back();

		records[h.id].gen++;
		records[h.id].nextFree = freeHead;
		freeHead = h.id;
	}

	void tick() {
		
		// for ( aliveIndex) TODO

	}

};
