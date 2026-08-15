#pragma once

#include <def/vec3.h>
#include <def/quat.h>
// #include <def/mtx.h>


struct Transform {
	vec3f pos;
	Quat rot;
	vec3f scale;
	// Mtx mtx;

	// Transform operator*(const Transform& rhs) const;
	// void rebuildMatrix();

	Transform(): pos{}, rot{}, scale{1.0f, 1.0f, 1.0f} {};
};
