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

	Mtx operator*(const Mtx& other);

	void setPos(const vec3f& pos);
	void setScale(const vec3f& scale);
	vec3f pos();
	const vec3f pos() const;

	// 行列から回転成分を抽出する。PoseSolver など、回転だけを必要とする処理で使う。
	Quat rot() const;
	vec3f scale() const;

	// 行列を位置・回転・スケールに分解して Transform に変換する。
	// Transform は TRS のみを保持するため、shear を含む行列は完全には再現できない。
	Transform toTransform() const;

	static Mtx fromTRS(const Transform& trs);

	static Mtx inverse(const Mtx& target);
};
