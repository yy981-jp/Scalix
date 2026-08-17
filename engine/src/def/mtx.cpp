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

void decomposeRotScale(
	const float* m,
	bx::Vec3& outScale,
	bx::Quaternion& outRot)
{
	// row-vector convention:
	// S * R * T では、回転基底は rows に入っている。
	bx::Vec3 row0 = { m[0], m[4], m[8]  };
	bx::Vec3 row1 = { m[1], m[5], m[9]  };
	bx::Vec3 row2 = { m[2], m[6], m[10] };

	float sx = bx::length(row0);
	float sy = bx::length(row1);
	float sz = bx::length(row2);

	if (sx < 1e-8f || sy < 1e-8f || sz < 1e-8f) {
		outScale = { sx, sy, sz };
		outRot = { 0.0f, 0.0f, 0.0f, 1.0f };
		return;
	}

	float det =
		  m[0] * (m[5] * m[10] - m[6] * m[9])
		- m[4] * (m[1] * m[10] - m[2] * m[9])
		+ m[8] * (m[1] * m[6] - m[2] * m[5]);

	if (det < 0.0f) {
		sx = -sx;
	}

	outScale = { sx, sy, sz };

	const float invSx = 1.0f / sx;
	const float invSy = 1.0f / sy;
	const float invSz = 1.0f / sz;

	const float r00 = row0.x * invSx;
	const float r01 = row0.y * invSx;
	const float r02 = row0.z * invSx;

	const float r10 = row1.x * invSy;
	const float r11 = row1.y * invSy;
	const float r12 = row1.z * invSy;

	const float r20 = row2.x * invSz;
	const float r21 = row2.y * invSz;
	const float r22 = row2.z * invSz;

	float trace = r00 + r11 + r22;

	float qx, qy, qz, qw;

	if (trace > 0.0f) {
		const float s = 0.5f / bx::sqrt(trace + 1.0f);
		qw = 0.25f / s;
		qx = (r21 - r12) * s;
		qy = (r02 - r20) * s;
		qz = (r10 - r01) * s;
	}
	else if (r00 > r11 && r00 > r22) {
		const float s = 2.0f * bx::sqrt(1.0f + r00 - r11 - r22);
		qw = (r21 - r12) / s;
		qx = 0.25f * s;
		qy = (r01 + r10) / s;
		qz = (r02 + r20) / s;
	}
	else if (r11 > r22) {
		const float s = 2.0f * bx::sqrt(1.0f + r11 - r00 - r22);
		qw = (r02 - r20) / s;
		qx = (r01 + r10) / s;
		qy = 0.25f * s;
		qz = (r12 + r21) / s;
	}
	else {
		const float s = 2.0f * bx::sqrt(1.0f + r22 - r00 - r11);
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
    float t[16], r[16], s[16], tmp[16];

    bx::mtxIdentity(t);
    bx::mtxIdentity(r);
    bx::mtxIdentity(s);

    t[12] = trs.pos.x;
    t[13] = trs.pos.y;
    t[14] = trs.pos.z;

    Quat q = Quat::inverse(trs.rot);

    bx::mtxFromQuaternion(r, q);

    s[0] = trs.scale.x;
    s[5] = trs.scale.y;
    s[10] = trs.scale.z;

    Mtx mtx;
    bx::mtxMul(tmp, s, r);
    bx::mtxMul(mtx.data(), tmp, t);

    return mtx;
}

Mtx Mtx::inverse(const Mtx& target) {
	Mtx res;
	bx::mtxInverse(res.data(), target.data());
	return res;
}
