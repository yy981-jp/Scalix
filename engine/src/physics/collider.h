#pragma once
#include <cstdint>
#include <vector>
#include <variant>

#include <physics/colliderDef.h>
#include <core/nodeRegistry.h>


struct Contact {
	ColliderHandle a;
	ColliderHandle b;

	vec3f point;
	vec3f normal;
	float penetration;
};


class ColliderSystem {
	struct Entry {
		ColliderId nextFree;
		uint32_t gen;
	};

	std::vector<Contact> collisions;
	
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

	// Global cache
	// 毎フレーム更新するため、サイズ管理は不要
	std::vector<WorldCollider> wcls;


	bool collide(const ColliderId& a_id, const ColliderId& b_id) {
		const auto& a = wcls[a_id];
		const auto& b = wcls[b_id];
		bool colide = std::visit(WSH::CollideVisitor{},a.shape,b.shape);
		if (!colide) return false;

		ColliderHandle a_hdl = {a_id, records[a_id].gen};
		ColliderHandle b_hdl = {b_id, records[b_id].gen};

		Contact collision{
			.a = a_hdl,
			.b = b_hdl,
		};

		collisions.push_back( std::move(collision) );
		return true;
	}


public:
	const std::vector<Contact>& getCollisions() const { return collisions; }

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
			colliders.emplace_back();
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


	bool isAlive(const ColliderHandle& hdl) const {
		return hdl.id < records.size()
				&& records[hdl.id].gen == hdl.gen;
	}


	void updateWorldShapes() {
		wcls.resize(records.size());

		for (ColliderId id: alive) {
			const Collider& collider = colliders[id];
			const Mtx& mtx = nodeReg.getGlobalMtx(collider.node);

			WorldCollider wcl;
			// WorldCollider::shape を構築
			wcl = std::visit(WSH::UpdateVisitor{mtx}, collider.data);

			wcls[id] = std::move(wcl);
			
		}
	}

	std::vector<bool> broadPhase() {
		std::vector<bool> enable;
		enable.resize(alive.size(),true);
		// TODO
		return enable;
	}

	void tick() {
		collisions.clear();
		updateWorldShapes();
		std::vector<bool> enable = broadPhase();
		
		size_t aliveSize = alive.size();
		// ia < ib を保証する
		for (size_t ib = 0; ib < aliveSize; ib++) {
			if (!enable[ib]) continue;
			const ColliderId& b = alive[ib];
			for (size_t ia = 0; ia < ib; ia++) {
				if (!enable[ia]) continue;
				const ColliderId& a = alive[ia];

				collide(a,b);
			}
		}
	};



};
