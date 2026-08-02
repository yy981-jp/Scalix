#include <tracker/poseSolver.h>
#include <core/avatar.h>

#include <anim/loader.h>
#include <gltf/loader.h>
#include <util/cache.h>
#include <util/math.h>
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

	if (has(ctx.keyStat, KCode::T))
		ctx.poseSolver->requestCalibration();

	status = (walking? Status::walk : Status::stay);

	// --- アニメーション ---
	anim.tick(*this,dt);


	// --- 視点 ---
	if (has(ctx.keyStat, KCode::n0)) ctx.cam_type = CameraType::DEBUG;
	else if (has(ctx.keyStat, KCode::n1)) ctx.cam_type = CameraType::_1;

	const float yawDelta = ctx.mStat.relPos.x * sensitivity;
	head.yaw   += yawDelta;
	head.pitch -= ctx.mStat.relPos.y * sensitivity;

	// 制限
	if (head.pitch >  headPitchLimit) head.pitch =  headPitchLimit;
	if (head.pitch < -headPitchLimit) head.pitch = -headPitchLimit;

	float clampedHeadYaw = head.yaw;
	if (clampedHeadYaw > headYawLimit) {
		clampedHeadYaw = headYawLimit;
	}
	if (clampedHeadYaw < -headYawLimit) {
		clampedHeadYaw = -headYawLimit;
	}
	if (head.yaw != clampedHeadYaw) {
		yaw -= head.yaw - clampedHeadYaw;
		head.yaw = clampedHeadYaw;
	}

	// --- neck (pitch) ---
	if (humanoid.has(HBT::neck)) {
		NodeHandle neckHandle = humanoid.bones[(size_t)HBT::neck];
		auto& neckNode = nodeReg.get(neckHandle);

		float qPitch[4];
		neckNode.trs.rot.setAxisAngle({1,0,0}, head.pitch);
	}

	if (humanoid.has(HBT::head)) {
		// --- head (yaw) ---
		NodeHandle headHandle = humanoid.bones[(size_t)HBT::head];
		auto& headNode = nodeReg.get(headHandle);

		headNode.trs.rot.setAxisAngle({0,1,0}, head.yaw);

		// printf("hn.rot[1]: %g, [2]: %g, [3]: %g, [4]: %g\n", headNode.rot[1], headNode.rot[2], headNode.rot[3], headNode.rot[4]);
	}
}


void Avatar::draw(Camera& cam) {
	// if (!humanoid.has(HBT::neck)) return;

	NodeHandle headHandle = humanoid.bones[(size_t)HBT::head];
	const Node& headNode = nodeReg.get(headHandle);

	int headIdx = nodeReg.getId(headHandle);
	if (headIdx < 0) return;

	// globalTransform のTRS成分は非一様スケール／鏡映を完全には表せないため、
	// 描画と同じグローバル行列から視点の位置・向きを取得する。
	const auto& headMtx = globalTransforms[headIdx].mtx;
	const vec3f headPos{headMtx[12], headMtx[13], headMtx[14]};
	const vec3f lookDir{bx::normalize(bx::mulXyz0({0.0f, 0.0f, 1.0f}, headMtx.data()))};


	vec3f camPos = headPos
		+ lookDir * 0.1f
		+ vec3f{0, 0.05f, 0};

	cam.update(camPos, camPos + lookDir);
}

Avatar::Avatar(const std::string& glTFPath, AvatarId id): id(id) {
	pos = {0, 0, 0};
	yaw = 0.0f;
	head = {};

	GltfLoaderImpl loader(glTFPath);
	loader.load();
	loader.procHdl(this);
	model = loader.get();


	humanoid.init(model);

	// nodeReg.get(humanoid.bones[static_cast<int>(HBT::arm_left_up)]).trs.rot.setAxisAngle({1,0,0}, deg2rad(65)); // deg=65
	// nodeReg.get(humanoid.bones[static_cast<int>(HBT::arm_right_up)]).trs.rot.setAxisAngle({1,0,0}, deg2rad(65));

	anim.init("test.sxa",*this);

	// anim.run("Sweater_OFF"_hs);
}

Avatar::~Avatar() {
	for (const NodeHandle& hdl: model.nodeHandles) {
		nodeReg.destroy(hdl);
	}
}
