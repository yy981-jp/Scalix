#pragma once
#include <bx/math.h>


struct vec3f {
    float x, y, z;

    vec3f() : x(0), y(0), z(0) {}
    vec3f(float x, float y, float z) : x(x), y(y), z(z) {}
    vec3f(const bx::Vec3& v): x(v.x), y(v.y), z(v.z) {};

    // bx用の変換関数 (出力)
    operator bx::Vec3() const;

    // bx用の変換関数 (入力)
    vec3f& operator=(const bx::Vec3& v);

    // 演算
    vec3f operator+(const vec3f& other) const;
    vec3f operator-(const vec3f& other) const;
    vec3f& operator+=(const vec3f& other);
    vec3f operator-() const;

    vec3f operator*(float scalar) const;
    vec3f operator/(float scalar) const;
};
