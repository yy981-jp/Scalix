#include <def/vec3.h>

#include <limits.h>


vec3f::operator bx::Vec3() const {
	return bx::Vec3{x, y, z};
}

vec3f::operator FormatDef::vec3f() const {
	return {x, y, z};
}

constexpr vec3f& vec3f::invalid() {
	static float fnan = std::numeric_limits<float>::quiet_NaN();
	static vec3f invalid = {fnan, fnan, fnan};
	return invalid;
};

bool vec3f::isValid() {
	return !std::isnan(x) || !std::isnan(y) || !std::isnan(z);
}


// 演算
vec3f vec3f::operator+(const vec3f& other) const {
	return {
		x + other.x,
		y + other.y,
		z + other.z
	};
}

vec3f vec3f::operator-(const vec3f& other) const {
	return {
		x - other.x,
		y - other.y,
		z - other.z
	};
}

vec3f& vec3f::operator+=(const vec3f& other) {
	x += other.x;
	y += other.y;
	z += other.z;
	return *this;
}

vec3f& vec3f::operator-=(const vec3f& other) {
	x -= other.x;
	y -= other.y;
	z -= other.z;
	return *this;
}

vec3f vec3f::operator-() const  {
	return *this * -1;
}



vec3f vec3f::operator*(float scalar) const {
	return {
		x * scalar,
		y * scalar,
		z * scalar
	};
}

vec3f vec3f::operator*(const vec3f& other) const {
	return {
		x * other.x,
		y * other.y,
		z * other.z
	};
}

vec3f operator*(float s, const vec3f& v) {
	return v * s;
}

vec3f vec3f::operator/(float scalar) const {
	return {
		x / scalar,
		y / scalar,
		z / scalar
	};
}

vec3f vec3f::lerp(const vec3f& a, const vec3f& b, float t) {
	return bx::lerp(a,b,t);
}

vec3f vec3f::cross(const vec3f& a, const vec3f& b) {
	return bx::cross(a,b);
}
