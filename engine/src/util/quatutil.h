#pragma once

#include <def/vec3.h>


void quatRotateAxis(float* out, float x, float y, float z, float angle);
void quatMul(float* out, const float* a, const float* b);
void quatNormalize(float* q);
void quatFromTo(float* out, const vec3f& from, const vec3f& to);
