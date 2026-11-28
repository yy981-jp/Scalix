#include <def/vec3.h>
#include <def/mtx.h>


class Collidertag {};
using ColliderId = Id<Collidertag>;
using ColliderGen = uint32_t;
struct ColliderHandle {
	ColliderId id;
	ColliderGen gen;
};
namespace std {
	template<>
	struct hash<ColliderHandle> {
		size_t operator()(const ColliderHandle& hdl) const noexcept {
			return static_cast<size_t>(hdl.id ^ hdl.gen);
		}
	};
}


struct Contact {
	ColliderHandle other;

	// vec3f point;
	vec3f normal;
	float penetration;
};


struct DynamicBitset {
	std::vector<uint64_t> words;

	void set(size_t index) {
		const size_t word = index / 64;
		const size_t bit = index % 64;

		if (words.size() <= word) {
			words.resize(word + 1);
		}

		words[word] |= uint64_t{1} << bit;
	}
};


// ==============================
//			Local
// ==============================


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
};

using ColliderData = std::variant<CLSH::Sphere,CLSH::Capsule>;

// struct ColliderLayer {
// 	// invalid = 0

// 	ColliderLayer(): value(0) {}
// 	ColliderLayer(uint64_t layerId): value(layerId) {}
// 	uint64_t value;

// 	static ColliderLayer latest;
// 	static ColliderLayer createNewLayer() {
// 		ColliderLayer layer = ++latest.value;
// 		return layer;
// 	}
// };

using ColliderLayer = DynamicBitset;

struct Collider {
	NodeHandle node;
	ColliderData data;
	ColliderLayer layer; // 自分
	ColliderLayer mask; // 衝突相手
};



// ==============================
//			Global
// ==============================

struct WorldCollider;

namespace WSH {
	struct Sphere {
		vec3f center{};
		float radius = 0.0f;
	};

	struct Capsule {
		vec3f head{};
		vec3f tail{};
		float radius = 0.0f;
	};



	#define func(Type) WorldCollider operator()(const CLSH::Type& col) const;
	struct UpdateVisitor {
		const Mtx& mtx;

		func(Sphere)
		func(Capsule)
	};
	#undef func

	#define func(AType,BType) bool operator()(const AType& a, const BType& b);
	#define func_m(AType,BType) func(BType,AType); func(AType,BType);
	struct CollideVisitor {
		vec3f normal;
		float penetration = 0;

		func(Sphere,Sphere)
		func_m(Sphere,Capsule)
		func(Capsule,Capsule)
	};
	#undef func
	#undef func_m

}

using WorldShape = std::variant<
	WSH::Sphere,
	WSH::Capsule
>;

// narrowphaseで用いる、純粋な空間や形状の情報を持つデータ
struct WorldCollider {
	WorldShape shape;
	// AABB bounds;
};


// ==============================
//			End Shapes
// ==============================
