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


Quat Quat::lerp(const Quat& a, const Quat& b, float t) {
	return bx::lerp(a,b,t);
}

void Quat::setAxisAngle(const vec3f& axis, float angle) {
	float half = angle * 0.5f;
	float s = lutsv.getSin(half);

	x = axis.x * s;
	y = axis.y * s;
	z = axis.z * s;
	w = lutsv.getCos(half);
}
