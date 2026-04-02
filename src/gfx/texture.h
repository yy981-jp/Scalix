#pragma once
#include <bgfx/bgfx.h>
#include <string>
#include <stdexcept>

struct Texture {
	bgfx::TextureHandle handle{};
	bgfx::UniformHandle sampler{};
	int width = 0;
	int height = 0;

	Texture() = default;
	~Texture() = default;

	// 画像データから直接作成
	void createFromRGBA(int w, int h, const uint8_t* rgba_data, size_t size) {
		if (!rgba_data || size == 0) {
			throw std::runtime_error("Invalid texture data");
		}

		width = w;
		height = h;

		const bgfx::Memory* mem = bgfx::copy(rgba_data, size);
		
		handle = bgfx::createTexture2D(
			(uint16_t)w,
			(uint16_t)h,
			false,
			1,
			bgfx::TextureFormat::RGBA8,
			BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT,
			mem
		);

		if (!bgfx::isValid(handle)) {
			throw std::runtime_error("Failed to create texture");
		}

		// サンプラーユニフォームを作成（一度だけ）
		if (!bgfx::isValid(sampler)) {
			sampler = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);
		}
	}

	// RGB画像からRGBA変換して作成
	void createFromRGB(int w, int h, const uint8_t* rgb_data, size_t size) {
		if (!rgb_data) {
			throw std::runtime_error("Invalid texture data");
		}

		// RGB -> RGBA変換
		std::vector<uint8_t> rgba(w * h * 4);
		for (int i = 0; i < w * h; i++) {
			rgba[i * 4 + 0] = rgb_data[i * 3 + 0];
			rgba[i * 4 + 1] = rgb_data[i * 3 + 1];
			rgba[i * 4 + 2] = rgb_data[i * 3 + 2];
			rgba[i * 4 + 3] = 255;
		}

		createFromRGBA(w, h, rgba.data(), rgba.size());
	}

	void bind(uint8_t stage = 0) {
		if (bgfx::isValid(handle) && bgfx::isValid(sampler)) {
			bgfx::setTexture(stage, sampler, handle);
		}
	}

	bool isValid() const {
		return bgfx::isValid(handle);
	}

	void release() {
		if (bgfx::isValid(handle)) {
			bgfx::destroy(handle);
			handle = {};
		}
		if (bgfx::isValid(sampler)) {
			bgfx::destroy(sampler);
			sampler = {};
		}
	}
};