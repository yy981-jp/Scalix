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
void decomposeRotScale(const float* m, bx::Vec3& outScale, bx::Quaternion& outRot) {
	bx::Vec3 row0{m[0], m[1], m[2]};
	bx::Vec3 row1{m[4], m[5], m[6]};
	bx::Vec3 row2{m[8], m[9], m[10]};

	const float sx = bx::length(row0);
	const float sy = bx::length(row1);
	const float sz = bx::length(row2);
	outScale = {sx, sy, sz};

	row0 = sx > bx::kNearZero ? bx::mul(row0, 1.0f / sx) : bx::Vec3{1.0f, 0.0f, 0.0f};
	row1 = sy > bx::kNearZero ? bx::mul(row1, 1.0f / sy) : bx::Vec3{0.0f, 1.0f, 0.0f};
	row2 = sz > bx::kNearZero ? bx::mul(row2, 1.0f / sz) : bx::Vec3{0.0f, 0.0f, 1.0f};

	const float trace = row0.x + row1.y + row2.z;
	if (trace > 0.0f) {
		const float s = bx::sqrt(trace + 1.0f) * 2.0f;
		outRot.w = 0.25f * s;
		// 標準的なShepperd法は qx=(m21-m12)/s だが、m21=row2.y, m12=row1.z なので
		// (row2.y - row1.z)/s が正しい。符号が逆だとx/y/zだけが反転した
		// 「共役四元数(=逆回転)」になってしまい、wはそのままなのでq≡-qの
		// 符号反転(無害)では吸収できず、実際に逆方向の回転として現れる。
		// trace>0は通常の(小さい)回転角でほぼ必ず通るブランチなので、
		// この符号ミスがPoseSolverの「反応はするが向きがおかしい」原因だった。
		outRot.x = (row2.y - row1.z) / s;
		outRot.y = (row0.z - row2.x) / s;
		outRot.z = (row1.x - row0.y) / s;
	} else if (row0.x > row1.y && row0.x > row2.z) {
		// 同様に qw=(m21-m12)/s = (row2.y-row1.z)/s が正しい向き。
		// x/y/zは元々正しかったが、wだけ符号が逆だと(x,y,z,-w)は
		// 「-1倍しても共役と一致しない」ため無害なq≡-qの符号反転では吸収できず、
		// 実質 conjugate(q)(=逆回転)と同じ回転になってしまっていた。
		const float s = bx::sqrt(1.0f + row0.x - row1.y - row2.z) * 2.0f;
		outRot.w = (row2.y - row1.z) / s;
		outRot.x = 0.25f * s;
		outRot.y = (row1.x + row0.y) / s;
		outRot.z = (row2.x + row0.z) / s;
	} else if (row1.y > row2.z) {
		const float s = bx::sqrt(1.0f + row1.y - row0.x - row2.z) * 2.0f;
		outRot.w = (row0.z - row2.x) / s;
		outRot.x = (row1.x + row0.y) / s;
		outRot.y = 0.25f * s;
		outRot.z = (row2.y + row1.z) / s;
	} else {
		const float s = bx::sqrt(1.0f + row2.z - row0.x - row1.y) * 2.0f;
		outRot.w = (row1.x - row0.y) / s;
		outRot.x = (row2.x + row0.z) / s;
		outRot.y = (row2.y + row1.z) / s;
		outRot.z = 0.25f * s;
	}
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
