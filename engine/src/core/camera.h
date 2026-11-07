#pragma once


#include <def/vec3.h>


enum class CameraType {
	_1, _2, _3,
	DEBUG, // 固定位置
};


class Camera {
	int WIDTH, HEIGHT;
	float SceneAspect;
	float proj[16];

public:
	void init(int width, int height, float near_);
	void update(const vec3f& pos, const vec3f& lookAt);
};
