#pragma once
#include <stdexcept>
#include <bgfx/bgfx.h>


class BGFXMan {
public:
	void init(const bgfx::Init& param) {
		if (!bgfx::init(param)) throw std::runtime_error("bgfx init failed");
	}

	~BGFXMan() {
		bgfx::shutdown();
	}
};
