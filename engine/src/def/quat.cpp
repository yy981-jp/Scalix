#include <bx/math.h>
#include <def/quat.h>

#include <util/cache.h>
#include <def/vec3.h>


Quat::operator bx::Quaternion() const {
	return {x, y, z, w};
}

Quat::operator FormatDef::Quat() const {
	return {x, y, z, w};
}

// 演算
Quat Quat::operator+(const Quat& other) const {
	return {
		x + other.x,
		y + other.y,
		z + other.z,
		w + other.w
	};
}

Quat Quat::operator-(const Quat& other) const {
	return {
		x - other.x,
		y - other.y,
		z - other.z,
		w - other.w
	};
}

Quat& Quat::operator+=(const Quat& other) {
	x += other.x;
	y += other.y;
	z += other.z;
	w += other.w;
	return *this;
}

Quat& Quat::operator-=(const Quat& other) {
	x -= other.x;
	y -= other.y;
	z -= other.z;
	w -= other.w;
	return *this;
}

Quat Quat::operator-() const  {
	return *this * -1;
}



Quat Quat::operator*(float scalar) const {
	return {
		x * scalar,
		y * scalar,
		z * scalar,
		w * scalar
	};
}

Quat Quat::operator*(const Quat& other) const {
	return {
		w * other.x + x * other.w + y * other.z - z * other.y,
		w * other.y - x * other.z + y * other.w + z * other.x,
		w * other.z + x * other.y - y * other.x + z * other.w,
		w * other.w - x * other.x - y * other.y - z * other.z
	};
}

Quat Quat::operator/(float scalar) const {
	return {
		x / scalar,
		y / scalar,
		z / scalar,
		w / scalar
	};
}

vec3f Quat::operator*(const vec3f& v) const {
	vec3f u{x, y, z};

	vec3f t = vec3f::cross(u, v) * 2.0f;
	return v + w * t + vec3f::cross(u, t);
}

Quat& Quat::operator*=(const Quat& other) {
	*this = *this * other;
	return *this;
}


void Quat::normalize() {
	*this = bx::normalize(*this);
}

float* Quat::data() {
	return &x;
}



Quat Quat::lerp(const Quat& a, const Quat& b, float t) {
	return bx::lerp(a,b,t);
}

Quat Quat::fromAxisAngle(const vec3f& axis, float rad) {
	float half = rad * 0.5f;
	float s = lutsv.getSin(half);

	Quat res;
	res.x = axis.x * s;
	res.y = axis.y * s;
	res.z = axis.z * s;
	res.w = lutsv.getCos(half);

	return res;
}

Quat Quat::fromTo(vec3f from, vec3f to) {
	from.normalize();
	to.normalize();

	float dot = bx::dot(from, to);

	// ほぼ同じ方向
	if (dot > 0.9999f) return {0,0,0,1};

	// 逆向き
	if (dot < -0.9999f) {
		// 適当な直交軸を作る
		vec3f axis = bx::cross({1,0,0}, from);
		if (bx::length(axis) < 0.0001f)
			axis = bx::cross({0,1,0}, from);

		axis = bx::normalize(axis);
		return fromAxisAngle(axis, bx::kPi);
	}

	vec3f axis = bx::cross(from, to);
	axis = bx::normalize(axis);

	float angle = acosf(dot);

	Quat res = fromAxisAngle(axis, angle);
	res.normalize();

	return res;
}
