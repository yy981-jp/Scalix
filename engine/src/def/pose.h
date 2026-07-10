#pragma once

#include <def/str.h>
#include <def/vec3.h>
#include <def/quat.h>


struct Pose {
    vec3f pos;
    Quat rot;
    vec3f scale;
};
