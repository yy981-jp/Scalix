#include <gfx/texture.h>
#include <vector>

namespace {
	bgfx::UniformHandle g_sampler = BGFX_INVALID_HANDLE;
}

void Texture::createFromRGBA(int w, int h, const uint8_t* rgba_data, size_t size) {
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

	// サンプラーユニフォームを作成（一度だけ、全テクスチャ共有）
	if (!bgfx::isValid(g_sampler)) {
		g_sampler = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);
	}
	sampler = g_sampler;
}

// RGB画像からRGBA変換して作成
void Texture::createFromRGB(int w, int h, const uint8_t* rgb_data, size_t size) {
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

void Texture::bind(uint8_t stage) {
	bgfx::setTexture(stage, sampler, handle);
}

bool Texture::isValid() const {
	return bgfx::isValid(handle);
}

Texture::Texture(Texture&& other) noexcept {
	handle = other.handle;
	other.handle = BGFX_INVALID_HANDLE;
	sampler = other.sampler;
	other.sampler = BGFX_INVALID_HANDLE;
	width = other.width;
	height = other.height;
}

Texture& Texture::operator=(Texture&& other) noexcept {
	if (this != &other) {

		if (bgfx::isValid(handle)) {
			bgfx::destroy(handle);
		}

		handle = other.handle;
		other.handle = BGFX_INVALID_HANDLE;
		sampler = other.sampler;
		other.sampler = BGFX_INVALID_HANDLE;
		width = other.width;
		height = other.height;
	}
	return *this;
}


Texture::~Texture() {
	if (bgfx::isValid(handle)) {
		bgfx::destroy(handle);
		handle = BGFX_INVALID_HANDLE;
	}
}
