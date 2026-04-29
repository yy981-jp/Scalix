#include "key.h"
#include <SDL.h>

const std::unordered_map<SDL_Keycode, KCode> keyMap = {
	{ SDLK_a, KCode::A },
	{ SDLK_b, KCode::B },
	{ SDLK_c, KCode::C },
	{ SDLK_d, KCode::D },
	{ SDLK_e, KCode::E },
	{ SDLK_f, KCode::F },
	{ SDLK_g, KCode::G },
	{ SDLK_h, KCode::H },
	{ SDLK_i, KCode::I },
	{ SDLK_j, KCode::J },
	{ SDLK_k, KCode::K },
	{ SDLK_l, KCode::L },
	{ SDLK_m, KCode::M },
	{ SDLK_n, KCode::N },
	{ SDLK_o, KCode::O },
	{ SDLK_p, KCode::P },
	{ SDLK_q, KCode::Q },
	{ SDLK_r, KCode::R },
	{ SDLK_s, KCode::S },
	{ SDLK_t, KCode::T },
	{ SDLK_u, KCode::U },
	{ SDLK_v, KCode::V },
	{ SDLK_w, KCode::W },
	{ SDLK_x, KCode::X },
	{ SDLK_y, KCode::Y },
	{ SDLK_z, KCode::Z },

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
