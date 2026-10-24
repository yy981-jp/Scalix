#include <ui/rml.h>

#include <def/view.h>
#include <util/image.h>
#include <util/path.h>

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <vector>


namespace
{
	bool loadRmlUiTexture(
		const Rml::String& source,
		Rml::Vector2i& dimensions,
		std::vector<Rml::byte>& rgba_pixels)
	{
		dimensions.x = 0;
		dimensions.y = 0;
		rgba_pixels.clear();

		fs::path path = fs::u8path(source);
		if (path.is_relative())
			path = syspath.resource / path;
		path = path.lexically_normal();

		std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> decoded(
			loadImageSurface(path),
			SDL_DestroySurface
		);
		if (!decoded)
		{
			SDL_Log(
				"RmlUi: failed to load image '%s': %s",
				source.c_str(),
				SDL_GetError()
			);
			return false;
		}

		bool has_color_key = SDL_SurfaceHasColorKey(decoded.get());
		Uint32 color_key = 0;
		Uint8 key_r = 0;
		Uint8 key_g = 0;
		Uint8 key_b = 0;
		Uint8 key_a = 255;
		if (has_color_key)
		{
			const bool got_key = SDL_GetSurfaceColorKey(decoded.get(), &color_key);
			const SDL_PixelFormatDetails* format =
				SDL_GetPixelFormatDetails(decoded->format);
			if (!got_key || format == nullptr)
			{
				has_color_key = false;
			}
			else
			{
				SDL_GetRGBA(
					color_key,
					format,
					SDL_GetSurfacePalette(decoded.get()),
					&key_r,
					&key_g,
					&key_b,
					&key_a
				);
			}
		}

		std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> rgba(
			SDL_ConvertSurface(decoded.get(), SDL_PIXELFORMAT_RGBA32),
			SDL_DestroySurface
		);
		if (!rgba)
		{
			SDL_Log(
				"RmlUi: failed to convert image '%s' to RGBA32: %s",
				source.c_str(),
				SDL_GetError()
			);
			return false;
		}

		const int width = rgba->w;
		const int height = rgba->h;
		if (width <= 0 || height <= 0 ||
			width > std::numeric_limits<uint16_t>::max() ||
			height > std::numeric_limits<uint16_t>::max())
		{
			SDL_Log("RmlUi: image '%s' has unsupported dimensions (%d x %d).",
				source.c_str(), width, height);
			return false;
		}

		const size_t pixel_width = static_cast<size_t>(width);
		const size_t pixel_height = static_cast<size_t>(height);
		if (pixel_width > std::numeric_limits<size_t>::max() / 4)
			return false;

		const size_t row_bytes = pixel_width * 4;
		if (pixel_height > std::numeric_limits<size_t>::max() / row_bytes)
			return false;

		const size_t pixel_bytes = row_bytes * pixel_height;
		if (rgba->pitch < 0 || static_cast<size_t>(rgba->pitch) < row_bytes)
		{
			SDL_Log("RmlUi: image '%s' has an invalid row pitch.", source.c_str());
			return false;
		}

		rgba_pixels.resize(pixel_bytes);
		if (!SDL_LockSurface(rgba.get()))
		{
			rgba_pixels.clear();
			SDL_Log(
				"RmlUi: failed to access image '%s': %s",
				source.c_str(),
				SDL_GetError()
			);
			return false;
		}

		const auto* source_pixels = static_cast<const Rml::byte*>(rgba->pixels);
		for (size_t y = 0; y < pixel_height; ++y)
		{
			std::memcpy(
				rgba_pixels.data() + y * row_bytes,
				source_pixels + y * static_cast<size_t>(rgba->pitch),
				row_bytes
			);
		}
		SDL_UnlockSurface(rgba.get());

		for (size_t offset = 0; offset < pixel_bytes; offset += 4)
		{
			Rml::byte* pixel = rgba_pixels.data() + offset;
			if (has_color_key &&
				pixel[0] == key_r &&
				pixel[1] == key_g &&
				pixel[2] == key_b)
			{
				pixel[3] = 0;
			}

			// RmlUi's renderer uses premultiplied-alpha blending.
			const unsigned alpha = pixel[3];
			pixel[0] = static_cast<Rml::byte>((pixel[0] * alpha + 127u) / 255u);
			pixel[1] = static_cast<Rml::byte>((pixel[1] * alpha + 127u) / 255u);
			pixel[2] = static_cast<Rml::byte>((pixel[2] * alpha + 127u) / 255u);
		}

		dimensions.x = width;
		dimensions.y = height;
		(void)key_a;
		return true;
	}
}


RmlUiLayer::RmlUiLayer(int width, int height)
{
	scalix::ui::RmlUiRendererBgfxConfig config = {
		.view_id = static_cast<bgfx::ViewId>(ViewId::ui),
		.width = width,
		.height = height,
		.shader_directory = syspath.resource / "runtime",
		.texture_loader = loadRmlUiTexture
	};
	backend.init(config);
}
