#include <ui/rml.h>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/ElementDocument.h>

#include <def/view.h>
#include <util/image.h>
#include <util/path.h>

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
bool loadRmlUiTexture(const Rml::String &source, Rml::Vector2i &dimensions,
						std::vector<Rml::byte> &rgba_pixels) {
	dimensions.x = 0;
	dimensions.y = 0;
	rgba_pixels.clear();

	fs::path path = fs::u8path(source);
	if (path.is_relative())
		path = syspath.resource / path;
	path = path.lexically_normal();

	std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> decoded(
			loadImageSurface(path), SDL_DestroySurface);
	if (!decoded) {
		SDL_Log("RmlUi: failed to load image '%s': %s", source.c_str(), SDL_GetError());
		return false;
	}

	bool has_color_key = SDL_SurfaceHasColorKey(decoded.get());
	Uint32 color_key = 0;
	Uint8 key_r = 0;
	Uint8 key_g = 0;
	Uint8 key_b = 0;
	if (has_color_key) {
		const bool got_key = SDL_GetSurfaceColorKey(decoded.get(), &color_key);
		const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(decoded->format);
		if (!got_key || format == nullptr) {
			has_color_key = false;
		} else {
			Uint8 key_a = 255;
			SDL_GetRGBA(color_key, format, SDL_GetSurfacePalette(decoded.get()),
			&key_r, &key_g, &key_b, &key_a);
		}
	}

	std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> rgba(
			SDL_ConvertSurface(decoded.get(), SDL_PIXELFORMAT_RGBA32),
			SDL_DestroySurface);
	if (!rgba) {
		SDL_Log("RmlUi: failed to convert image '%s' to RGBA32: %s", source.c_str(),
						SDL_GetError());
		return false;
	}

	const int width = rgba->w;
	const int height = rgba->h;
	if (width <= 0 || height <= 0 ||
			width > std::numeric_limits<uint16_t>::max() ||
			height > std::numeric_limits<uint16_t>::max()) {
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
	if (rgba->pitch < 0 || static_cast<size_t>(rgba->pitch) < row_bytes) {
		SDL_Log("RmlUi: image '%s' has an invalid row pitch.", source.c_str());
		return false;
	}

	rgba_pixels.resize(pixel_bytes);
	if (!SDL_LockSurface(rgba.get())) {
		rgba_pixels.clear();
		SDL_Log("RmlUi: failed to access image '%s': %s", source.c_str(),
						SDL_GetError());
		return false;
	}

	const auto *source_pixels = static_cast<const Rml::byte *>(rgba->pixels);
	for (size_t y = 0; y < pixel_height; ++y) {
		std::memcpy(rgba_pixels.data() + y * row_bytes,
								source_pixels + y * static_cast<size_t>(rgba->pitch),
								row_bytes);
	}
	SDL_UnlockSurface(rgba.get());

	for (size_t offset = 0; offset < pixel_bytes; offset += 4) {
		Rml::byte *pixel = rgba_pixels.data() + offset;
		if (has_color_key && pixel[0] == key_r && pixel[1] == key_g &&
				pixel[2] == key_b) {
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
	return true;
}

int toRmlModifiers(SDL_Keymod modifiers) {
	int result = 0;
	if (modifiers & SDL_KMOD_CTRL)
		result |= Rml::Input::KM_CTRL;
	if (modifiers & SDL_KMOD_SHIFT)
		result |= Rml::Input::KM_SHIFT;
	if (modifiers & SDL_KMOD_ALT)
		result |= Rml::Input::KM_ALT;
	if (modifiers & SDL_KMOD_GUI)
		result |= Rml::Input::KM_META;
	if (modifiers & SDL_KMOD_CAPS)
		result |= Rml::Input::KM_CAPSLOCK;
	if (modifiers & SDL_KMOD_NUM)
		result |= Rml::Input::KM_NUMLOCK;
	if (modifiers & SDL_KMOD_SCROLL)
		result |= Rml::Input::KM_SCROLLLOCK;
	return result;
}

int toRmlMouseButton(Uint8 button) {
	switch (button) {
	case SDL_BUTTON_LEFT:
		return 0;
	case SDL_BUTTON_RIGHT:
		return 1;
	case SDL_BUTTON_MIDDLE:
		return 2;
	case SDL_BUTTON_X1:
		return 3;
	case SDL_BUTTON_X2:
		return 4;
	default:
		return -1;
	}
}

Rml::Input::KeyIdentifier toRmlKey(SDL_Scancode key) {
	using Rml::Input::KeyIdentifier;

	if (key >= SDL_SCANCODE_A && key <= SDL_SCANCODE_Z) {
		return static_cast<KeyIdentifier>(Rml::Input::KI_A + (key - SDL_SCANCODE_A));
	}
	if (key >= SDL_SCANCODE_1 && key <= SDL_SCANCODE_9) {
		return static_cast<KeyIdentifier>(Rml::Input::KI_1 + (key - SDL_SCANCODE_1));
	}
	if (key >= SDL_SCANCODE_F1 && key <= SDL_SCANCODE_F12) {
		return static_cast<KeyIdentifier>(Rml::Input::KI_F1 + (key - SDL_SCANCODE_F1));
	}
	if (key >= SDL_SCANCODE_F13 && key <= SDL_SCANCODE_F24) {
		return static_cast<KeyIdentifier>(Rml::Input::KI_F13 + (key - SDL_SCANCODE_F13));
	}
	if (key >= SDL_SCANCODE_KP_1 && key <= SDL_SCANCODE_KP_9) {
		return static_cast<KeyIdentifier>(Rml::Input::KI_NUMPAD1 + (key - SDL_SCANCODE_KP_1));
	}

	switch (key) {
	case SDL_SCANCODE_SPACE:
		return Rml::Input::KI_SPACE;
	case SDL_SCANCODE_0:
		return Rml::Input::KI_0;
	case SDL_SCANCODE_SEMICOLON:
		return Rml::Input::KI_OEM_1;
	case SDL_SCANCODE_EQUALS:
		return Rml::Input::KI_OEM_PLUS;
	case SDL_SCANCODE_COMMA:
		return Rml::Input::KI_OEM_COMMA;
	case SDL_SCANCODE_MINUS:
		return Rml::Input::KI_OEM_MINUS;
	case SDL_SCANCODE_PERIOD:
		return Rml::Input::KI_OEM_PERIOD;
	case SDL_SCANCODE_SLASH:
		return Rml::Input::KI_OEM_2;
	case SDL_SCANCODE_GRAVE:
		return Rml::Input::KI_OEM_3;
	case SDL_SCANCODE_LEFTBRACKET:
		return Rml::Input::KI_OEM_4;
	case SDL_SCANCODE_BACKSLASH:
		return Rml::Input::KI_OEM_5;
	case SDL_SCANCODE_RIGHTBRACKET:
		return Rml::Input::KI_OEM_6;
	case SDL_SCANCODE_APOSTROPHE:
		return Rml::Input::KI_OEM_7;
	case SDL_SCANCODE_NONUSBACKSLASH:
		return Rml::Input::KI_OEM_102;

	case SDL_SCANCODE_KP_0:
		return Rml::Input::KI_NUMPAD0;
	case SDL_SCANCODE_KP_ENTER:
		return Rml::Input::KI_NUMPADENTER;
	case SDL_SCANCODE_KP_MULTIPLY:
		return Rml::Input::KI_MULTIPLY;
	case SDL_SCANCODE_KP_PLUS:
		return Rml::Input::KI_ADD;
	case SDL_SCANCODE_KP_MINUS:
		return Rml::Input::KI_SUBTRACT;
	case SDL_SCANCODE_KP_PERIOD:
		return Rml::Input::KI_DECIMAL;
	case SDL_SCANCODE_KP_DIVIDE:
		return Rml::Input::KI_DIVIDE;
	case SDL_SCANCODE_KP_EQUALS:
		return Rml::Input::KI_OEM_NEC_EQUAL;
	case SDL_SCANCODE_KP_COMMA:
		return Rml::Input::KI_SEPARATOR;

	case SDL_SCANCODE_BACKSPACE:
		return Rml::Input::KI_BACK;
	case SDL_SCANCODE_TAB:
		return Rml::Input::KI_TAB;
	case SDL_SCANCODE_CLEAR:
		return Rml::Input::KI_CLEAR;
	case SDL_SCANCODE_RETURN:
		return Rml::Input::KI_RETURN;
	case SDL_SCANCODE_PAUSE:
		return Rml::Input::KI_PAUSE;
	case SDL_SCANCODE_CAPSLOCK:
		return Rml::Input::KI_CAPITAL;
	case SDL_SCANCODE_ESCAPE:
		return Rml::Input::KI_ESCAPE;
	case SDL_SCANCODE_LANG1:
		return Rml::Input::KI_HANGUL;
	case SDL_SCANCODE_LANG2:
		return Rml::Input::KI_HANJA;
	case SDL_SCANCODE_INTERNATIONAL2:
		return Rml::Input::KI_KANA;
	case SDL_SCANCODE_INTERNATIONAL4:
		return Rml::Input::KI_CONVERT;
	case SDL_SCANCODE_INTERNATIONAL5:
		return Rml::Input::KI_NONCONVERT;
	case SDL_SCANCODE_PAGEUP:
		return Rml::Input::KI_PRIOR;
	case SDL_SCANCODE_PAGEDOWN:
		return Rml::Input::KI_NEXT;
	case SDL_SCANCODE_END:
		return Rml::Input::KI_END;
	case SDL_SCANCODE_HOME:
		return Rml::Input::KI_HOME;
	case SDL_SCANCODE_LEFT:
		return Rml::Input::KI_LEFT;
	case SDL_SCANCODE_UP:
		return Rml::Input::KI_UP;
	case SDL_SCANCODE_RIGHT:
		return Rml::Input::KI_RIGHT;
	case SDL_SCANCODE_DOWN:
		return Rml::Input::KI_DOWN;
	case SDL_SCANCODE_SELECT:
		return Rml::Input::KI_SELECT;
	case SDL_SCANCODE_PRINTSCREEN:
		return Rml::Input::KI_SNAPSHOT;
	case SDL_SCANCODE_INSERT:
		return Rml::Input::KI_INSERT;
	case SDL_SCANCODE_DELETE:
		return Rml::Input::KI_DELETE;
	case SDL_SCANCODE_HELP:
		return Rml::Input::KI_HELP;
	case SDL_SCANCODE_LGUI:
#if defined(__APPLE__)
		return Rml::Input::KI_LMETA;
#else
		return Rml::Input::KI_LWIN;
#endif
	case SDL_SCANCODE_RGUI:
#if defined(__APPLE__)
		return Rml::Input::KI_RMETA;
#else
		return Rml::Input::KI_RWIN;
#endif
	case SDL_SCANCODE_APPLICATION:
		return Rml::Input::KI_APPS;

	case SDL_SCANCODE_NUMLOCKCLEAR:
		return Rml::Input::KI_NUMLOCK;
	case SDL_SCANCODE_SCROLLLOCK:
		return Rml::Input::KI_SCROLL;
	case SDL_SCANCODE_LSHIFT:
		return Rml::Input::KI_LSHIFT;
	case SDL_SCANCODE_RSHIFT:
		return Rml::Input::KI_RSHIFT;
	case SDL_SCANCODE_LCTRL:
		return Rml::Input::KI_LCONTROL;
	case SDL_SCANCODE_RCTRL:
		return Rml::Input::KI_RCONTROL;
	case SDL_SCANCODE_LALT:
		return Rml::Input::KI_LMENU;
	case SDL_SCANCODE_RALT:
		return Rml::Input::KI_RMENU;
	case SDL_SCANCODE_MUTE:
		return Rml::Input::KI_VOLUME_MUTE;
	case SDL_SCANCODE_VOLUMEDOWN:
		return Rml::Input::KI_VOLUME_DOWN;
	case SDL_SCANCODE_VOLUMEUP:
		return Rml::Input::KI_VOLUME_UP;
	default:
		return Rml::Input::KI_UNKNOWN;
	}
}
} // namespace

RmlUiLayer::~RmlUiLayer() { shutdown(); }

void RmlUiLayer::init(int width, int height) {
	if (initialized)
		throw std::logic_error("RmlUiLayer is already initialized.");
	if (width <= 0 || height <= 0)
		throw std::invalid_argument("RmlUiLayer requires positive dimensions.");

	scalix::ui::RmlUiRendererBgfxConfig config = {
		.view_id = static_cast<bgfx::ViewId>(ViewId::ui),
		.width = width,
		.height = height,
		.shader_directory = syspath.resource / "runtime",
		.texture_loader = loadRmlUiTexture
	};
	backend.init(config);

	const auto view_id = static_cast<bgfx::ViewId>(ViewId::ui);
	bgfx::setViewName(view_id, "Scalix UI");

	Rml::SetRenderInterface(&backend);
	if (!Rml::Initialise()) {
		Rml::SetRenderInterface(nullptr);
		backend.shutdown();
		throw std::runtime_error("RmlUi initialization failed.");
	}

	initialized = true;
	context = Rml::CreateContext("scalix_ui", Rml::Vector2i{width, height});
	if (!context) {
		shutdown();
		throw std::runtime_error("Failed to create the RmlUi context.");
	}
}

Rml::ElementDocument* RmlUiLayer::loadDocument(const fs::path& path) {
	if (!initialized || context == nullptr)
		throw std::logic_error("RmlUiLayer is not initialized.");
	if (path.empty())
		throw std::invalid_argument("RmlUi document path must not be empty.");

	fs::path document_path = path;
	if (document_path.is_relative())
		document_path = syspath.resource / document_path;
	document_path = document_path.lexically_normal();

	const auto path_utf8 = document_path.u8string();
	const std::string path_string(
		reinterpret_cast<const char*>(path_utf8.data()),
		path_utf8.size()
	);

	Rml::ElementDocument* document = context->LoadDocument(path_string);
	if (document == nullptr) {
		SDL_Log("RmlUi: failed to load document '%s'.", path_string.c_str());
		return nullptr;
	}

	document->Show();
	return document;
}

void RmlUiLayer::processEvent(const SDL_Event &event) {
	if (!initialized || context == nullptr)
		return;

	switch (event.type) {
	case SDL_EVENT_MOUSE_MOTION:
		context->ProcessMouseMove(static_cast<int>(event.motion.x),
			static_cast<int>(event.motion.y),
			toRmlModifiers(SDL_GetModState()));
		break;

	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	case SDL_EVENT_MOUSE_BUTTON_UP: {
		const int button = toRmlMouseButton(event.button.button);
		if (button < 0)
			break;

		const int modifiers = toRmlModifiers(SDL_GetModState());
		if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
			context->ProcessMouseButtonDown(button, modifiers);
		else
			context->ProcessMouseButtonUp(button, modifiers);
		break;
	}

	case SDL_EVENT_MOUSE_WHEEL: {
		float wheel_x = event.wheel.x;
		float wheel_y = event.wheel.y;
		if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
			wheel_x = -wheel_x;
			wheel_y = -wheel_y;
		}

		context->ProcessMouseWheel(Rml::Vector2f{wheel_x, wheel_y},
			toRmlModifiers(SDL_GetModState()));
		break;
	}

	case SDL_EVENT_KEY_DOWN:
	case SDL_EVENT_KEY_UP: {
		const auto key = toRmlKey(event.key.scancode);
		if (key == Rml::Input::KI_UNKNOWN)
			break;

		const int modifiers = toRmlModifiers(event.key.mod);
		if (event.type == SDL_EVENT_KEY_DOWN)
			context->ProcessKeyDown(key, modifiers);
		else
			context->ProcessKeyUp(key, modifiers);
		break;
	}

	case SDL_EVENT_TEXT_INPUT:
		if (event.text.text != nullptr && event.text.text[0] != '\0')
			context->ProcessTextInput(Rml::String(event.text.text));
		break;

	case SDL_EVENT_WINDOW_MOUSE_LEAVE:
	case SDL_EVENT_WINDOW_FOCUS_LOST:
		context->ProcessMouseLeave();
		break;

	default:
		break;
	}
}

void RmlUiLayer::update() {
	if (initialized && context != nullptr)
		context->Update();
}

void RmlUiLayer::render() {
	if (!initialized || context == nullptr)
		return;

	const auto view_id = static_cast<bgfx::ViewId>(ViewId::ui);
	// UI mode is rendered as a separate view, so it clears its own background.
	bgfx::setViewClear(view_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff,
										 1.0f, 0);
	bgfx::touch(view_id);
	context->Render();
}

void RmlUiLayer::shutdown() noexcept {
	if (initialized) {
		context = nullptr;
		Rml::Shutdown();
		initialized = false;
	}

	backend.shutdown();
}
