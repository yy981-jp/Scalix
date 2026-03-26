#pragma once
#include <bgfx/bgfx.h>

struct Texture {
	bgfx::TextureHandle handle{};
	bgfx::UniformHandle sampler{};

	void create(int w, int h, int comp, const unsigned char* data, size_t size) {
		handle = bgfx::createTexture2D(
			(uint16_t)w, (uint16_t)h,
			false, 1,
			comp == 4 ? bgfx::TextureFormat::RGBA8 : bgfx::TextureFormat::RGB8,
			0,
			bgfx::copy(data, size)
		);

		sampler = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);
	}

	void bind(uint8_t stage = 0) {
		bgfx::setTexture(stage, sampler, handle);
	}
};