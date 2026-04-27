#pragma once


#include <bgfx/bgfx.h>


class Camera {
    const int WIDTH, HEIGHT;
	const float SceneAspect;

public:
    Camera(int width, int height):
        WIDTH(width), HEIGHT(height),
        SceneAspect((float)width / (float)height) {}


    void update(const bx::Vec3& pos, const bx::Vec3& dir) {
        float view[16];
        float proj[16];
        bx::mtxLookAt(view,
            pos,
            {pos.x + dir.x, pos.y + dir.y, pos.z + dir.z}
        );

        bx::mtxProj(proj, 60.0f, SceneAspect, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);

        bgfx::setViewTransform(0, view, proj);
    }
};
