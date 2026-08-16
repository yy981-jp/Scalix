#include <def/mtx.h>


Mtx::Mtx() {
	// m_data.fill(0.f);
}

float* Mtx::data() {return m_data.data(); }
const float* Mtx::data() const {return m_data.data(); }

Mtx Mtx::operator*(const Mtx& other) {
	Mtx res;
	bx::mtxMul(res.data(), other.data(), data());
	return res;
}

void Mtx::setPos(const vec3f& pos) {
	m_data[12] = pos.x;
	m_data[13] = pos.y;
	m_data[14] = pos.z;
}
void Mtx::setScale(const vec3f& scale) {
	m_data[0] = scale.x;
	m_data[5] = scale.y;
	m_data[10] = scale.z;
}
vec3f Mtx::pos() { return {m_data[12], m_data[13], m_data[14]}; }
const vec3f Mtx::pos() const { return {m_data[12], m_data[13], m_data[14]}; }

namespace {
// この Mtx は row-vector 規約 (v' = v * M) で、row0-2 が基底ベクトル、
// row3 (m[12..14]) が平行移動。fromTRS() は S * R * T の順で合成しているため、
// 3x3 部分の各行は「スケール量 * 回転後の基底ベクトル」になっている。
// スケールを取り除く(=各行を正規化する)ことで純粋な回転行列を復元し、
// それを四元数に変換する(Shepperd/標準的なmatrix->quaternion手法)。
void decomposeRotScale(const float* m, bx::Vec3& outScale, bx::Quaternion& outRot)
{
    // Column-major layout: column c occupies m[c*4 .. c*4+3]
    // column 0 = X axis * scaleX
    // column 1 = Y axis * scaleY
    // column 2 = Z axis * scaleZ
    bx::Vec3 col0 = { m[0],  m[1],  m[2]  };
    bx::Vec3 col1 = { m[4],  m[5],  m[6]  };
    bx::Vec3 col2 = { m[8],  m[9],  m[10] };

    float sx = bx::length(col0);
    float sy = bx::length(col1);
    float sz = bx::length(col2);

    // Negative determinant -> one axis is mirrored (fold sign into scaleX)
    float det =
          m[0] * (m[5] * m[10] - m[6] * m[9])
        - m[4] * (m[1] * m[10] - m[2] * m[9])
        + m[8] * (m[1] * m[6]  - m[2] * m[5]);

    if (det < 0.0f)
    {
        sx = -sx;
    }

    outScale = { sx, sy, sz };

    // Normalize columns to isolate the pure rotation part
    float invSx = 1.0f / sx;
    float invSy = 1.0f / sy;
    float invSz = 1.0f / sz;

    // r[row][col], column-major indices
    float r00 = col0.x * invSx, r10 = col0.y * invSx, r20 = col0.z * invSx;
    float r01 = col1.x * invSy, r11 = col1.y * invSy, r21 = col1.z * invSy;
    float r02 = col2.x * invSz, r12 = col2.y * invSz, r22 = col2.z * invSz;

    // Trace-based matrix -> quaternion conversion
    float trace = r00 + r11 + r22;
    float qx, qy, qz, qw;

    if (trace > 0.0f)
    {
        float s = 0.5f / bx::sqrt(trace + 1.0f);
        qw = 0.25f / s;
        qx = (r21 - r12) * s;
        qy = (r02 - r20) * s;
        qz = (r10 - r01) * s;
    }
    else if (r00 > r11 && r00 > r22)
    {
        float s = 2.0f * bx::sqrt(1.0f + r00 - r11 - r22);
        qw = (r21 - r12) / s;
        qx = 0.25f * s;
        qy = (r01 + r10) / s;
        qz = (r02 + r20) / s;
    }
    else if (r11 > r22)
    {
        float s = 2.0f * bx::sqrt(1.0f + r11 - r00 - r22);
        qw = (r02 - r20) / s;
        qx = (r01 + r10) / s;
        qy = 0.25f * s;
        qz = (r12 + r21) / s;
    }
    else
    {
        float s = 2.0f * bx::sqrt(1.0f + r22 - r00 - r11);
        qw = (r10 - r01) / s;
        qx = (r02 + r20) / s;
        qy = (r12 + r21) / s;
        qz = 0.25f * s;
    }

    outRot = { qx, qy, qz, qw };
}
}

Quat Mtx::rot() const {
	bx::Vec3 scale{1.0f, 1.0f, 1.0f};
	bx::Quaternion rotation{0.0f, 0.0f, 0.0f, 1.0f};
	decomposeRotScale(m_data.data(), scale, rotation);
	return Quat(rotation);
}

vec3f Mtx::scale() const {
	bx::Vec3 scale{1.0f, 1.0f, 1.0f};
	bx::Quaternion rotation{0.0f, 0.0f, 0.0f, 1.0f};
	decomposeRotScale(m_data.data(), scale, rotation);
	return {scale.x, scale.y, scale.z};
}

Transform Mtx::toTransform() const {
	bx::Vec3 scale{1.0f, 1.0f, 1.0f};
	bx::Quaternion rotation{0.0f, 0.0f, 0.0f, 1.0f};
	decomposeRotScale(m_data.data(), scale, rotation);

	Transform t;
	t.pos = pos();
	t.rot = Quat(rotation);
	t.scale = {scale.x, scale.y, scale.z};
	return t;
}

Mtx Mtx::fromTRS(const Transform& trs) {
	float t[16], r[16], s[16], tmp[16], tmp2[16];

	bx::mtxIdentity(t);
	bx::mtxIdentity(r);
	bx::mtxIdentity(s);

	t[12] = trs.pos.x;  t[13] = trs.pos.y;  t[14] = trs.pos.z;

	// if (hasRot) {
	bx::mtxFromQuaternion(r, trs.rot);
	// }

	s[0] = trs.scale.x;  s[5] = trs.scale.y;  s[10] = trs.scale.z;


	Mtx mtx;
	bx::mtxMul(tmp, s, r);	   // R * S
	bx::mtxMul(mtx.data(), tmp, t);	 // (S * R) * T
	return mtx;
}

Mtx Mtx::inverse(const Mtx& target) {
	Mtx res;
	bx::mtxInverse(res.data(), target.data());
	return res;
}
