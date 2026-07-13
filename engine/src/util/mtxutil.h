#pragma once

#include <bx/math.h>
#include <def/vec3.h>
#include <def/quat.h>

// TRS行列を組み立てるヘルパー
inline void buildTRS(float* out, const vec3f& pos, const Quat& rot,
			  const vec3f& scale, bool hasRot = true) {
	float t[16], r[16], s[16], tmp[16];

	bx::mtxIdentity(t);
	bx::mtxIdentity(r);
	bx::mtxIdentity(s);

	t[12] = pos.x;  t[13] = pos.y;  t[14] = pos.z;

	if (hasRot) {
		bx::mtxFromQuaternion(r, rot);
	}

	s[0] = scale.x;  s[5] = scale.y;  s[10] = scale.z;

	bx::mtxMul(tmp, s, r);	   // R * S
	bx::mtxMul(out, tmp, t);	 // (S * R) * T
};
