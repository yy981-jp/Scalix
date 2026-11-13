#pragma once

#include <array>
#include <bx/bx.h>
#include <bx/math.h>

#include <def/quat.h>
#include <def/transform.h>


class Mtx {
	std::array<float, 16> m_data;

public:
	Mtx();

	float* data();
	const float* data() const;

	Mtx operator*(const Mtx& other) const;

	vec3f pos() const;

	// rot / scale は TRS 分解による取得。shear を含む行列では近似値になる。
	Quat rot() const;
	vec3f scale() const;

	void setPos(const vec3f& pos);

	// setRot / setScale / rotate / scaleBy は TRS 分解・再構成を行うため、shear を保持しない。
	void setRot(const Quat& rot);
	void setScale(const vec3f& scale);

	// translate は現在の回転方向を基準にしたローカル移動。スケールは移動量に適用しない。
	void translate(const vec3f& offset);

	// rotate は現在の回転にローカル回転を追加する。
	void rotate(const Quat& rot);

	// scaleBy は各軸のスケールに倍率を適用する。
	void scaleBy(const vec3f& scale);

	// 行列を位置・回転・スケールに分解して Transform に変換する。
	// Transform は TRS のみを保持するため、shear を含む行列は完全には再現できない。
	Transform toTransform() const;

	static Mtx fromTRS(const Transform& trs);

	static Mtx inverse(const Mtx& target);
};
