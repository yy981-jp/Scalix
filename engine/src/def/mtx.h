#pragma once

#include <array>
#include <bx/bx.h>
#include <bx/math.h>

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

	static Mtx fromTRS(const Transform& trs);

	static Mtx inverse(const Mtx& target);
};
