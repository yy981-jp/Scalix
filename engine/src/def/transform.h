#pragma once

#include <def/vec3.h>
#include <def/quat.h>


struct Transform {
	vec3f pos;
	Quat rot;
	vec3f scale;


	Transform(): pos{}, rot{}, scale{1.0f, 1.0f, 1.0f} {};
};
