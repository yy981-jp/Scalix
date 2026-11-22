#pragma once
#include <numbers>
#include <def/vec3.h>


constexpr float deg2rad(float deg) {
	return deg * (std::numbers::pi / 180.0f);
}

constexpr float rad2deg(float rad) {
	return rad * (180.0f / std::numbers::pi);
}

constexpr float maxAbsComponent(const vec3f& value ) {
	return std::max({
		std::abs(value.x),
		std::abs(value.y),
		std::abs(value.z)
	});
}

constexpr float length(const vec3f& value) {
	return bx::length(value);
}

constexpr float dot(const vec3f& a, const vec3f& b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

constexpr float pointSegmentDistance(const vec3f& p, const vec3f& a, const vec3f& b) {
	const vec3f ab = b - a;
	const vec3f ap = p - a;

	const float denom = dot(ab, ab);

	// 線分の長さが0の場合
	if (denom <= 0.0f) {
		return length(ap);
	}

	float t = dot(ap, ab) / denom;
	t = std::clamp(t, 0.0f, 1.0f);

	const vec3f q = a + ab * t;
	const vec3f d = p - q;

	return std::sqrt(dot(d, d));
}
