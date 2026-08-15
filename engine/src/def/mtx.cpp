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
