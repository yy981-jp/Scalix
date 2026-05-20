#include "quat.h"


Quat::operator bx::Quaternion() const {
    return {x, y, z, w};
}

Quat::operator FormatDef::Quat() const {
    return {x, y, z, w};
}

// 演算
Quat Quat::operator+(const Quat& other) const {
    return {
        x + other.x,
        y + other.y,
        z + other.z,
        w + other.w
    };
}

Quat Quat::operator-(const Quat& other) const {
    return {
        x - other.x,
        y - other.y,
        z - other.z,
        w - other.w
    };
}

Quat& Quat::operator+=(const Quat& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return *this;
}

Quat& Quat::operator-=(const Quat& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return *this;
}

Quat Quat::operator-() const  {
    return *this * -1;
}



Quat Quat::operator*(float scalar) const {
    return {
        x * scalar,
        y * scalar,
        z * scalar,
        w * scalar
    };
}

Quat Quat::operator/(float scalar) const {
    return {
        x / scalar,
        y / scalar,
        z / scalar,
        w / scalar
    };
}



Quat Quat::lerp(const Quat& a, const Quat& b, float t) {
    return bx::lerp(a,b,t);
}
