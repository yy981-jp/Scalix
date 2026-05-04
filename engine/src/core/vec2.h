#pragma once

template<typename T>
struct Vec2 {
    T x, y;

    constexpr Vec2() : x(0), y(0) {}
    constexpr Vec2(T x, T y) : x(x), y(y) {}

    // 別の Vec2<U> から明示変換
    template<typename U>
    explicit constexpr Vec2(const Vec2<U>& v) : x(static_cast<T>(v.x)), y(static_cast<T>(v.y)) {}

    template<typename U>
    constexpr Vec2& operator=(const Vec2<U>& v) {
        x = static_cast<T>(v.x);
        y = static_cast<T>(v.y);
        return *this;
    }

    // 加算
    constexpr Vec2 operator+(const Vec2& other) const {
        return Vec2(x + other.x, y + other.y);
    }

    constexpr Vec2 operator-(const Vec2& other) const {
        return Vec2(x - other.x, y - other.y);
    }

    constexpr Vec2 operator-() const  {
        return *this * -1;
    }

    constexpr Vec2& operator+=(const Vec2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    // scaler乗算
    constexpr Vec2 operator*(T scalar) const {
        return Vec2(x * scalar, y * scalar);
    }

    constexpr Vec2 operator/(T scalar) const {
        return Vec2(x / scalar, y / scalar);
    }
};

// alias
using vec2f = Vec2<float>;
using vec2i = Vec2<int>;
