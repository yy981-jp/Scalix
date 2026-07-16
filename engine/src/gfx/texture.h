#pragma once
#include <bgfx/bgfx.h>
#include <string>
#include <stdexcept>


struct Texture {
	bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle sampler = BGFX_INVALID_HANDLE;
	int width = 0;
	int height = 0;

	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;

	Texture(Texture&& other) noexcept;
	Texture& operator=(Texture&& other) noexcept;

	Texture() = default;
	// ~Texture() = default;
	
	// 画像データから直接作成
	void createFromRGBA(int w, int h, const uint8_t* rgba_data, size_t size);

	// RGB画像からRGBA変換して作成
	void createFromRGB(int w, int h, const uint8_t* rgb_data, size_t size);

	void bind(uint8_t stage = 0);

	bool isValid() const;

	~Texture();
};