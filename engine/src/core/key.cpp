#include <core/key.h>
#include <SDL3/SDL.h>

const std::unordered_map<SDL_Keycode, KCode> keyMap = {
	{ SDLK_0, KCode::n0 },
	{ SDLK_1, KCode::n1 },
	{ SDLK_2, KCode::n2 },
	{ SDLK_3, KCode::n3 },
	{ SDLK_4, KCode::n4 },
	{ SDLK_5, KCode::n5 },
	{ SDLK_6, KCode::n6 },
	{ SDLK_7, KCode::n7 },
	{ SDLK_8, KCode::n8 },
	{ SDLK_9, KCode::n9 },

	{ SDLK_A, KCode::A },
	{ SDLK_B, KCode::B },
	{ SDLK_C, KCode::C },
	{ SDLK_D, KCode::D },
	{ SDLK_E, KCode::E },
	{ SDLK_F, KCode::F },
	{ SDLK_G, KCode::G },
	{ SDLK_H, KCode::H },
	{ SDLK_I, KCode::I },
	{ SDLK_J, KCode::J },
	{ SDLK_K, KCode::K },
	{ SDLK_L, KCode::L },
	{ SDLK_M, KCode::M },
	{ SDLK_N, KCode::N },
	{ SDLK_O, KCode::O },
	{ SDLK_P, KCode::P },
	{ SDLK_Q, KCode::Q },
	{ SDLK_R, KCode::R },
	{ SDLK_S, KCode::S },
	{ SDLK_T, KCode::T },
	{ SDLK_U, KCode::U },
	{ SDLK_V, KCode::V },
	{ SDLK_W, KCode::W },
	{ SDLK_X, KCode::X },
	{ SDLK_Y, KCode::Y },
	{ SDLK_Z, KCode::Z },

	{ SDLK_SPACE, KCode::Space },

	{ SDLK_LSHIFT, KCode::Shift },
	{ SDLK_RSHIFT, KCode::Shift },

	{ SDLK_LCTRL, KCode::Ctrl },
	{ SDLK_RCTRL, KCode::Ctrl },

	{ SDLK_ESCAPE, KCode::Esc },
	{ SDLK_TAB, KCode::Tab },

	{ SDLK_LALT, KCode::Alt },
	{ SDLK_RALT, KCode::Alt },

	{ SDLK_RETURN, KCode::Enter },
	{ SDLK_DELETE, KCode::Delete },
	{ SDLK_BACKSPACE, KCode::BkSpace },
};

const std::unordered_map<Uint8, MCode> mMap = {
	{ SDL_BUTTON_LEFT, MCode::Left},
	{ SDL_BUTTON_RIGHT, MCode::Right },
	{ SDL_BUTTON_MIDDLE, MCode::Mid }
};
