#include "vec3.h"

vec3f::operator bx::Vec3() const {
    return bx::Vec3{x, y, z};
}

// bx用の変換関数 (入力)
vec3f& vec3f::operator=(const bx::Vec3& v) {
    x = v.x;
    y = v.y;
    z = v.z;
    return *this;
}

// 加算
vec3f vec3f::operator+(const vec3f& other) const {
    return {
        x + other.x,
        y + other.y,
        z + other.z
    };
}

// 減算
vec3f vec3f::operator-(const vec3f& other) const {
    return {
        x - other.x,
        y - other.y,
        z - other.z
    };
}

vec3f vec3f::operator-() const  {
    return *this * -1;
}

vec3f& vec3f::operator+=(const vec3f& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

// scaler乗算
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
