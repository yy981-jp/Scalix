#pragma once

#include <bx/math.h>

// TRS行列を組み立てるヘルパー
void buildTRS(float* out, const float* pos, const float* rot,
              const float* scale, bool hasRot = true) {
    float t[16], r[16], s[16], tmp[16];

    bx::mtxIdentity(t);
    bx::mtxIdentity(r);
    bx::mtxIdentity(s);

    t[12] = pos[0];  t[13] = pos[1];  t[14] = pos[2];

    if (hasRot) {
        bx::Quaternion q = { rot[0], rot[1], rot[2], rot[3] };
        bx::mtxFromQuaternion(r, q);
    }

    s[0] = scale[0];  s[5] = scale[1];  s[10] = scale[2];

    // TRS順序: T * R * S
    bx::mtxMul(tmp, r, s);       // tmp = R * S
    bx::mtxMul(out, t, tmp);     // out = T * (R * S) = T * R * S
};

// DEBUG版：クォータニオン共役なし
inline void buildTRS_NoConjugate(float* out, const vec3f& pos, const float* rotOriginal,
                                  const float* scale, bool hasRot = true) {
    float t[16], r[16], s[16], tmp[16];

    bx::mtxIdentity(t);
    bx::mtxIdentity(r);
    bx::mtxIdentity(s);

    t[12] = pos.x;  t[13] = pos.y;  t[14] = pos.z;

    if (hasRot) {
        bx::Quaternion q = { rotOriginal[0], rotOriginal[1], rotOriginal[2], rotOriginal[3] };
        bx::mtxFromQuaternion(r, q);
    }

    s[0] = scale[0];  s[5] = scale[1];  s[10] = scale[2];

    // TRS順序: T * R * S
    bx::mtxMul(tmp, r, s);       // tmp = R * S
    bx::mtxMul(out, t, tmp);     // out = T * (R * S) = T * R * S
};
