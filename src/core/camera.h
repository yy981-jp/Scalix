#pragma once


#include "def.h"


enum class CameraType {
    _1, _2, _3,
    DEBUG, // 固定位置
};


class Camera {
    const int WIDTH, HEIGHT;
	const float SceneAspect;

public:
    Camera(int width, int height):
        WIDTH(width), HEIGHT(height),
        SceneAspect((float)width / (float)height) {}


    void update(const vec3f& pos, const vec3f& dir) {
        float view[16];
        float proj[16];
        bx::mtxLookAt(view,
            pos,
            pos + dir
        );

        bx::mtxProj(proj, 60.0f, SceneAspect, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);

        bgfx::setViewTransform(0, view, proj);
    }
};
