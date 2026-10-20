#pragma once

#include <physics/def.h>

class Jump {
	float velocity = 0.0f, height = 0.0f, groundHeight = 0.0f;
	static constexpr const float& gravity = def::gravity.y;
	bool grounded = true;

public:
	void jump(float initialVelocity) {
		if (!grounded) {
			return;
		}

		velocity = initialVelocity;
		grounded = false;
	}

	void tick(float dt) {
		if (grounded) {
			return;
		}

		velocity -= gravity * dt;
		height += velocity * dt;

		if (height <= 0.0f) {
			height = 0.0f;
			velocity = 0.0f;
			grounded = true;
		}
	}

	void setGroundHeight(float i_groundHeight) {
		groundHeight = i_groundHeight;
	}

	float getHeight() const {
		return height;
	}

	bool isGrounded() const {
		return grounded;
	}
};
