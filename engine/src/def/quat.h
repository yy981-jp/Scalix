#pragma once

#include <shared/format.h>
#include <bx/math.h>


struct vec3f;

struct Quat {
	float x, y, z, w;

	Quat() : x(0), y(0), z(0), w(1) {}
	Quat(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
	Quat(const bx::Quaternion& v): x(v.x), y(v.y), z(v.z), w(v.w) {};
	Quat(const FormatDef::Quat& v): x(v[0]), y(v[1]), z(v[2]), w(v[3]) {};

	operator bx::Quaternion() const;
	operator FormatDef::Quat() const;

	
	// 演算
	Quat operator+(const Quat& other) const;
	Quat operator-(const Quat& other) const;
	Quat& operator+=(const Quat& other);
	Quat& operator-=(const Quat& other);
	Quat operator-() const;
	
	Quat operator*(float scalar) const;
	Quat operator/(float scalar) const;

	vec3f operator*(const vec3f& v) const;

	static Quat lerp(const Quat& a, const Quat& b, float t);
	
	void setAxisAngle(const vec3f& axis, float angle);
};
