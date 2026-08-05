#pragma once

#include <array>
#include <bx/bx.h>
#include <bx/math.h>

#include <def/vec3.h>


class Mtx {
	std::array<float, 16> m_data;

public:
	Mtx() {
		m_data.fill(0.f);
	}

	float* data() {return m_data.data(); }
	const float* data() const {return m_data.data(); }

	Mtx operator*(const Mtx& other) {
		Mtx res;
		bx::mtxMul(res.data(), other.data(), data());
		return res;
	}

	void setPos(const vec3f& pos) {
		m_data[12] = pos.x;
		m_data[13] = pos.y;
		m_data[14] = pos.z;
	}
	vec3f pos() { return {m_data[12], m_data[13], m_data[14]}; }
	const vec3f pos() const { return {m_data[12], m_data[13], m_data[14]}; }
};
