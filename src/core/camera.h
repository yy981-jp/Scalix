#pragma once


#include "def.h"


enum class CameraType {
    _1, _2, _3,
    DEBUG, // 固定位置
};


class Camera {
    const int WIDTH, HEIGHT;
	const float SceneAspect;
    float proj[16];

public:
    Camera(int width, int height, float near_):
      WIDTH(width), HEIGHT(height),
      SceneAspect((float)width / (float)height) {
        bx::mtxProj(proj, 60.0f, SceneAspect, near_, 100.0f, bgfx::getCaps()->homogeneousDepth);
    }


    void update(const vec3f& pos, const vec3f& lookAt) {
        float view[16];
        bx::mtxLookAt(view,
            pos,
            lookAt
        );

        bgfx::setViewTransform(0, view, proj);
    }
};
