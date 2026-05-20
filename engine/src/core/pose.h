#pragma once

#include "str.h"
#include "vec3.h"
#include "quat.h"


struct Pose {
    vec3f pos;
    Quat rot;
    vec3f scale;
};
