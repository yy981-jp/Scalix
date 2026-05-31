#pragma once
#include <bx/math.h>
#include "../../../shared/format.h"


struct vec3f {
    float x, y, z;

    vec3f() : x(0), y(0), z(0) {}
    vec3f(float x, float y, float z) : x(x), y(y), z(z) {}
    vec3f(const bx::Vec3& v): x(v.x), y(v.y), z(v.z) {};
    vec3f(const FormatDef::vec3f& v): x(v[0]), y(v[1]), z(v[2]) {};

    operator bx::Vec3() const;
    operator FormatDef::vec3f() const;

    static constexpr vec3f& invalid();
    bool isValid();

    // 演算
    vec3f operator+(const vec3f& other) const;
    vec3f operator-(const vec3f& other) const;
    vec3f& operator+=(const vec3f& other);
    vec3f& operator-=(const vec3f& other);
    vec3f operator-() const;
    
    vec3f operator*(float scalar) const;
    vec3f operator/(float scalar) const;


    static vec3f lerp(const vec3f& a, const vec3f& b, float t);
};
