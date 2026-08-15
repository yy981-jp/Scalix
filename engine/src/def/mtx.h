#pragma once

#include <array>
#include <bx/bx.h>
#include <bx/math.h>

#include <def/quat.h>
#include <def/transform.h>
// #include <def/vec3.h>


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

	// bx::mtxDecompose でスケール/回転/平行移動を分解して回転(Quat)を取り出す。
	// PoseSolver などトラッキング側は Mtx ではなく回転そのものを必要とするため、
	// trackingMtx から都度これを使って取得する。
	Quat rot() const;
	vec3f scale() const;

	// Transform(pos/rot/scale)への変換。globalTransforms/trackingTransforms を廃止し
	// globalMtx/trackingMtx を単一のソースオブトゥルースにするためのヘルパー。
	Transform toTransform() const;

	static Mtx fromTRS(const Transform& trs);

	static Mtx inverse(const Mtx& target);
};
