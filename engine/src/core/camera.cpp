#include <core/camera.h>

#include <bgfx/bgfx.h>


void Camera::init(int width, int height, float near_) {
	WIDTH = width;
	HEIGHT = height;
	SceneAspect = (float)width / (float)height;
	bx::mtxProj(proj, 60.0f, SceneAspect, near_, 100.0f, bgfx::getCaps()->homogeneousDepth);
}


void Camera::update(const vec3f& pos, const vec3f& lookAt) {
	float view[16];
	bx::mtxLookAt(view,
		pos,
		lookAt
	);

	bgfx::setViewTransform(0, view, proj);
}
