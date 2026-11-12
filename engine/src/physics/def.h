#pragma once
#include <def/vec3.h>


namespace def {
	constexpr float gravity = 9.8;
	constexpr float epsilon = 1.0e-6f;
}

enum class ColliderShape {
	Sphere,
	Capsule
};

