#pragma once
#include <numbers>

constexpr float deg2rad(float deg) {
    return deg * (std::numbers::pi / 180.0f);
}

constexpr float rad2deg(float rad) {
    return rad * (180.0f / std::numbers::pi);
}
