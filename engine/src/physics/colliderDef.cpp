#include <physics/colliderDef.h>
#include <util/math.h>


// update
#define func(Type) WorldCollider WSH::UpdateVisitor::operator()(const CLSH::Type& col) const
func(Sphere) {
	WorldCollider wcl;
	WSH::Sphere wsh;

	wsh.center = col.offset * mtx;
	wsh.radius = col.radius * maxAbsComponent(mtx.scale());

	wcl.shape = wsh;
	return wcl;
}
func(Capsule) {
	WorldCollider wcl;
	WSH::Capsule wsh;

	wsh.head = col.offset * mtx;
	wsh.tail = col.tail * mtx;
	wsh.radius = col.radius * maxAbsComponent(mtx.scale());

	wcl.shape = wsh;
	return wcl;
}
#undef func





struct SegmentClosestResult {
	vec3f pointA;
	vec3f pointB;
	float distanceSquared;
};

SegmentClosestResult closestPoints(
	const vec3f& p0,
	const vec3f& p1,
	const vec3f& q0,
	const vec3f& q1
) {
	const vec3f u = p1 - p0;
	const vec3f v = q1 - q0;
	const vec3f w = p0 - q0;

	const float a = dot(u, u);
	const float b = dot(u, v);
	const float c = dot(v, v);
	const float d = dot(u, w);
	const float e = dot(v, w);

	float s = 0.0f;
	float t = 0.0f;

	if (a == 0.0f && c == 0.0f) {
		// 両方とも長さ0
	}
	else if (a == 0.0f) {
		// 線分1が点
		t = std::clamp(e / c, 0.0f, 1.0f);
	}
	else if (c == 0.0f) {
		// 線分2が点
		s = std::clamp(-d / a, 0.0f, 1.0f);
	}
	else {
		const float denom = a * c - b * b;

		// 平行でなければ、直線同士の最近接点を求める
		if (denom > 1e-6f * a * c) {
			s = std::clamp(
				(b * e - c * d) / denom,
				0.0f, 1.0f
			);
		}

		t = (b * s + e) / c;

		// 線分2の範囲外なら、境界に固定して線分1を再計算
		if (t < 0.0f) {
			t = 0.0f;
			s = std::clamp(-d / a, 0.0f, 1.0f);
		}
		else if (t > 1.0f) {
			t = 1.0f;
			s = std::clamp((b - d) / a, 0.0f, 1.0f);
		}
	}

	const vec3f pa = p0 + u * s;
	const vec3f pb = q0 + v * t;
	const vec3f delta = pa - pb;

	return {
		pa,
		pb,
		dot(delta, delta)
	};
}


// collide
#define func(AType,BType) bool WSH::CollideVisitor::operator()(const AType& a, const BType& b) const
#define func_m(AType,BType) func(BType,AType) { return this->operator()(b,a); } func(AType,BType)
func(Sphere,Sphere) {
	float rr = a.radius + b.radius;
	vec3f delta = a.center - b.center;
	return dot(delta,delta) < (rr * rr);
}
func_m(Sphere,Capsule) {
	float rr = a.radius + b.radius;
	const vec3f& p = a.center;
	const vec3f& seg_a = b.head;
	const vec3f& seg_b = b.tail;

	const vec3f ab = seg_b - seg_a;
	const vec3f ap = p - seg_a;

	const float denom = dot(ab, ab);

	float d;
	if (denom <= 0.0f) {
		d = dot(ap,ap);
	} else {
		float t = dot(ap, ab) / denom;
		t = std::clamp(t, 0.0f, 1.0f);

		const vec3f q = seg_a + ab * t;
		const vec3f delta = p - q;
		d = dot(delta, delta);
	}
	
	return d < (rr * rr);
}

func(Capsule,Capsule) {
	const auto result = closestPoints(a.head, a.tail, b.head, b.tail);
	float rr = a.radius + b.radius;

	return result.distanceSquared < (rr * rr);
}
#undef func
#undef func_m
