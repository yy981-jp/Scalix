#pragma once

#include <array>

#include <def/vec3.h>
#include <def/quat.h>


struct Transform {
	vec3f pos;
	Quat rot;
	vec3f scale;
	std::array<float,16> mtx;

	Transform operator*(const Transform& rhs) const;
	void rebuildMatrix();

	Transform();
};
