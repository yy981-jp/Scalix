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
