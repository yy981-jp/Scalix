#pragma once
#include <stdexcept>
#include <bgfx/bgfx.h>


class BGFXMan {
	bool initialized = false;

public:
	void init(const bgfx::Init& param) {
		if (initialized)
			throw std::logic_error("bgfx is already initialized");
		if (!bgfx::init(param))
			throw std::runtime_error("bgfx init failed");
		initialized = true;
	}

	void shutdown() noexcept {
		if (!initialized)
			return;
		bgfx::shutdown();
		initialized = false;
	}

	~BGFXMan() {
		shutdown();
	}
};
