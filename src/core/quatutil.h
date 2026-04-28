#pragma once
#include "cache.h"

#include <cmath>


void quatRotateAxis(float* out, float x, float y, float z, float angle) {
	float half = angle * 0.5f;
	float s = cachesv.getSin(half);

	out[0] = x * s;
	out[1] = y * s;
	out[2] = z * s;
	out[3] = cachesv.getCos(half);
}

void quatMul(float* out, const float* a, const float* b) {
	out[0] = a[3]*b[0] + a[0]*b[3] + a[1]*b[2] - a[2]*b[1];
	out[1] = a[3]*b[1] - a[0]*b[2] + a[1]*b[3] + a[2]*b[0];
	out[2] = a[3]*b[2] + a[0]*b[1] - a[1]*b[0] + a[2]*b[3];
	out[3] = a[3]*b[3] - a[0]*b[0] - a[1]*b[1] - a[2]*b[2];
}

void quatNormalize(float* q) {
	float len = sqrtf(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
	if (len > 0.0f) {
		float inv = 1.0f / len;
		q[0] *= inv;
		q[1] *= inv;
		q[2] *= inv;
		q[3] *= inv;
	}
}
