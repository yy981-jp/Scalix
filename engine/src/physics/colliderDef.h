#include <def/vec3.h>
#include <def/mtx.h>


class Collidertag {};
using ColliderId = Id<Collidertag>;
using ColliderGen = uint32_t;
struct ColliderHandle {
	ColliderId id;
	ColliderGen gen;
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

struct Collider {
	NodeHandle node;
	ColliderData data;
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

	#define func(AType,BType) bool operator()(const AType& a, const BType& b) const
	#define func_m(AType,BType) func(BType,AType); func(AType,BType)
	struct CollideVisitor {
		func(Sphere,Sphere);
		func_m(Sphere,Capsule);
		func(Capsule,Capsule);
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
