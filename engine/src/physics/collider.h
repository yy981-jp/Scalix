#pragma once
#include <cstdint>
#include <vector>
#include <variant>

#include <def/id.h>
#include <def/vec3.h>
#include <core/nodeRegistry.h>
#include <core/avatar.h>


class Collidertag {};
using ColliderId = Id<Collidertag>;
using ColliderGen = uint32_t;
struct ColliderHandle {
	ColliderId id;
	ColliderGen gen;
};


// Collider shape
namespace CLSH {
	
	struct Sphere {
		vec3f offset;
		float radius;
	};

	struct Capsule {
		vec3f offset;
		vec3f tail;

		float radius;
	};



	#define func(AType,BType) bool operator()(const AType& a, const BType& b) const
	#define func_m(AType,BType) func(BType,AType) { return this->operator()(b,a); } func(AType,BType)
	struct ShapeVisitor {
		func(Sphere,Sphere) {
			// TODO
		}
		func_m(Sphere,Capsule) {
			
		}
		func(Capsule,Capsule) {
			// TODO
		}
	};
	#undef func
	#undef func_m
};

using ColliderData = std::variant<CLSH::Sphere,CLSH::Capsule>;


template<typename T>
concept ColliderType = std::assignable_from<ColliderData&, T>;






struct Collider {
	NodeHandle node;
	ColliderData data;
};

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


	bool collide(const ColliderId& a_id, const ColliderId& b_id) {
		const auto& a = colliders[a_id];
		const auto& b = colliders[b_id];
		bool colide = std::visit(CLSH::ShapeVisitor{},a.data,b.data);
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

	void updateWorldShapes() {
		for (ColliderId id: alive) {
			const Collider& collider = colliders[id];
			// const Mtx& matrix = /* colliderに紐付くNodeのGlobal Mtx */;
			const auto& nodeEnt = nodeReg.getEntry(collider.node);
			// const Mtx& matrix = ;

			// ローカル形状をWorld-space形状に変換
		}
	}


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

	
	void tick() {
		collisions.clear();
		
		size_t aliveSize = alive.size();
		// ia < ib を保証する
		for (size_t ib = 0; ib < aliveSize; ib++) {
			const ColliderId& b = aliveIndex[ib];
			for (size_t ia = 0; ia < ib; ia++) {
				const ColliderId& a = aliveIndex[ia];

				collide(a,b);
			}
		}
	};



};
