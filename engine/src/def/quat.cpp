#include <def/quat.h>

#include <util/cache.h>


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

Quat Quat::operator/(float scalar) const {
	return {
		x / scalar,
		y / scalar,
		z / scalar,
		w / scalar
	};
}



Quat Quat::lerp(const Quat& a, const Quat& b, float t) {
	return bx::lerp(a,b,t);
}

void Quat::rotateAxis(float i_x, float i_y, float i_z, float angle) {
	float half = angle * 0.5f;
	float s = lutsv.getSin(half);

	x = i_x * s;
	y = i_y * s;
	z = i_z * s;
	w = lutsv.getCos(half);
}
