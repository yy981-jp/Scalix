#pragma once
#include <cstdint>
#include <unordered_map>
#include <SDL.h>
#include "def.h"


enum class KCode: uint64_t {
    n0      = 1ULL <<  0,
    n1      = 1ULL <<  1,
    n2      = 1ULL <<  2,
    n3      = 1ULL <<  3,
    n4      = 1ULL <<  4,
    n5      = 1ULL <<  5,
    n6      = 1ULL <<  6,
    n7      = 1ULL <<  7,
    n8      = 1ULL <<  8,
    n9      = 1ULL <<  9,
    A       = 1ULL << 10,
    B       = 1ULL << 11,
    C       = 1ULL << 12,
    D       = 1ULL << 13,
    E       = 1ULL << 14,
    F       = 1ULL << 15,
    G       = 1ULL << 16,
    H       = 1ULL << 17,
    I       = 1ULL << 18,
    J       = 1ULL << 19,
    K       = 1ULL << 20,
    L       = 1ULL << 21,
    M       = 1ULL << 22,
    N       = 1ULL << 23,
    O       = 1ULL << 24,
    P       = 1ULL << 25,
    Q       = 1ULL << 26,
    R       = 1ULL << 27,
    S       = 1ULL << 28,
    T       = 1ULL << 29,
    U       = 1ULL << 30,
    V       = 1ULL << 31,
    W       = 1ULL << 32,
    X       = 1ULL << 33,
    Y       = 1ULL << 34,
    Z       = 1ULL << 35,
    Space   = 1ULL << 36,
    Shift   = 1ULL << 37,
    Ctrl    = 1ULL << 38,
    Esc     = 1ULL << 39,
    Tab     = 1ULL << 40,
    Alt     = 1ULL << 41,
    Enter   = 1ULL << 42,
    Delete  = 1ULL << 43,
    BkSpace = 1ULL << 44,
    M_Left  = 1ULL << 45,
    M_Right = 1ULL << 46,
    M_Mid   = 1ULL << 47,
};

constexpr bool has(uint64_t keys, KCode k) {
    return keys & static_cast<uint64_t>(k);
}

extern const std::unordered_map<SDL_Keycode, KCode> keyMap;


struct MouseState {
    vec2f cursorRelPos = {0,0};
    vec2f cursorAbsPos = {0,0};
};
