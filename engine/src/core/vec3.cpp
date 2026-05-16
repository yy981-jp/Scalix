#include "vec3.h"

vec3f::operator bx::Vec3() const {
    return bx::Vec3{x, y, z};
}

vec3f::operator FormatDef::vec3f() const {
    return {x, y, z};
}

// 演算
vec3f vec3f::operator+(const vec3f& other) const {
    return {
        x + other.x,
        y + other.y,
        z + other.z
    };
}

vec3f vec3f::operator-(const vec3f& other) const {
    return {
        x - other.x,
        y - other.y,
        z - other.z
    };
}

vec3f& vec3f::operator+=(const vec3f& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

vec3f& vec3f::operator-=(const vec3f& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

vec3f vec3f::operator-() const  {
    return *this * -1;
}



vec3f vec3f::operator*(float scalar) const {
    return {
        x * scalar,
        y * scalar,
        z * scalar
    };
}

vec3f vec3f::operator/(float scalar) const {
    return {
        x / scalar,
        y / scalar,
        z / scalar
    };
}

vec3f vec3f::lerp(const vec3f& a, const vec3f& b, float t) {
    return bx::lerp(a,b,t);
}
