#include <util/quatutil.h>
#include <util/cache.h>

#include <cmath>


void quatRotateAxis(float* out, float x, float y, float z, float angle) {
	float half = angle * 0.5f;
	float s = lutsv.getSin(half);

	out[0] = x * s;
	out[1] = y * s;
	out[2] = z * s;
	out[3] = lutsv.getCos(half);
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

void quatFromTo(float* out, const vec3f& from, const vec3f& to) {
	vec3f f = bx::normalize(from);
	vec3f t = bx::normalize(to);

	float dot = bx::dot(f, t);

	// ほぼ同じ方向
	if (dot > 0.9999f) {
		out[0] = 0.0f;
		out[1] = 0.0f;
		out[2] = 0.0f;
		out[3] = 1.0f;
		return;
	}

	// 逆向き（これ重要）
	if (dot < -0.9999f) {
		// 適当な直交軸を作る
		vec3f axis = bx::cross({1,0,0}, f);
		if (bx::length(axis) < 0.0001f)
			axis = bx::cross({0,1,0}, f);

		axis = bx::normalize(axis);
		quatRotateAxis(out, axis.x, axis.y, axis.z, bx::kPi);
		return;
	}

	vec3f axis = bx::cross(f, t);
	axis = bx::normalize(axis);

	float angle = acosf(dot);

	quatRotateAxis(out, axis.x, axis.y, axis.z, angle);
	quatNormalize(out);
}
