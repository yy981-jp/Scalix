#include <core/avatar.h>

#include <anim/loader.h>
#include <gltf/loader.h>
#include <util/cache.h>
#include <core/key.h>
#include <core/gctx.h>
#include <util/quatutil.h>
#include <physics/springBone.h>
#include <core/nodeRegistry.h>

#include <cstdio>


void Avatar::update(GameContext& ctx, float dt) {
	// --- 移動 ---
	float fx = -lutsv.getSin(yaw);
	float fz =  lutsv.getCos(yaw);

	float rx =  fz;   // = cos(yaw)
	float rz = -fx;   // = sin(yaw)

	bool walking = false;

	float rlMove = 0.7;
	float aplSpeed = speed * dt;

	if (has(ctx.keyStat, KCode::W)) {
		walking = true;
		pos.x += fx * aplSpeed;
		pos.z += fz * aplSpeed;
	}
	if (has(ctx.keyStat, KCode::S)) {
		walking = true;
		pos.x -= fx * aplSpeed;
		pos.z -= fz * aplSpeed;
	}
	if (has(ctx.keyStat, KCode::A)) {
		walking = true;
		pos.x -= rx * aplSpeed * rlMove;
		pos.z -= rz * aplSpeed * rlMove;
	}
	if (has(ctx.keyStat, KCode::D)) {
		walking = true;
		pos.x += rx * aplSpeed * rlMove;
		pos.z += rz * aplSpeed * rlMove;
	}

	status = (walking? Status::walk : Status::stay);

	// --- アニメーション ---
	anim.tick(*this,dt);


	// --- 視点 ---
	if (has(ctx.keyStat, KCode::n0)) ctx.cam_type = CameraType::DEBUG;
	else if (has(ctx.keyStat, KCode::n1)) ctx.cam_type = CameraType::_1;

	head.yaw   += ctx.mStat.relPos.x * sensitivity;
	head.pitch -= ctx.mStat.relPos.y * sensitivity;

	// 制限
	if (head.pitch >  headPitchLimit) head.pitch =  headPitchLimit;
	if (head.pitch < -headPitchLimit) head.pitch = -headPitchLimit;

	if (head.yaw >  headYawLimit) {
		yaw -= head.yaw - headYawLimit; // 体を視点に追従
		head.yaw =  headYawLimit;
	}
	if (head.yaw < -headYawLimit) {
		yaw += -headYawLimit - head.yaw;
		head.yaw = -headYawLimit;
	}

	// --- neck (pitch) ---
	if (humanoid.has(HBT::neck)) {
		NodeHandle neckHandle = humanoid.bones[(size_t)HBT::neck];
		auto& neckNode = nodeReg.get(neckHandle);
		neckNode.hasRotation = true;

		float qPitch[4];
		neckNode.rot.setAxisAngle({1,0,0}, head.pitch);
	}

	if (humanoid.has(HBT::head)) {
		// --- head (yaw) ---
		NodeHandle headHandle = humanoid.bones[(size_t)HBT::head];
		auto& headNode = nodeReg.get(headHandle);
		headNode.hasRotation = true;

		headNode.rot.setAxisAngle({0,1,0}, head.yaw);

		// printf("hn.rot[1]: %g, [2]: %g, [3]: %g, [4]: %g\n", headNode.rot[1], headNode.rot[2], headNode.rot[3], headNode.rot[4]);
	}
}


void Avatar::draw(Camera& cam) {
	// if (!humanoid.has(HBT::neck)) return;

	NodeHandle headHandle = humanoid.bones[(size_t)HBT::head];
	const Node& headNode = nodeReg.get(headHandle);

	int headIdx = nodeReg.getId(headHandle);
	if (headIdx < 0) return;

	float* m = globalMtxs[headIdx].data();

	const vec3f& headPos = globalTransforms[headIdx].pos;

	// headの向きから直接forward取得
	// モデルによっては逆向きなので必要なら反転 (現状反転なし)
	const vec3f& lookDir = globalTransforms[headIdx].rot * vec3f{0,0,1};


	vec3f camPos = headPos
		+ lookDir * 0.1f
		+ vec3f{0, 0.05f, 0};

	cam.update(camPos, camPos + lookDir);
}

Avatar::Avatar(const std::string& glTFPath) {
	model = loadGltf(glTFPath);
	anim.init("test.sxa",*this);

	// anim.run("Sweater_OFF"_hs);
}
