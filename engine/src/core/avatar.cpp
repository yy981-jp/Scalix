#include <core/avatar.h>
#include <model/humanoid.h>

#include <anim/loader.h>
#include <loader/gltf.h>
#include <loader/vrm.h>
#include <util/cache.h>
#include <util/math.h>
#include <util/path.h>
#include <core/key.h>
#include <core/gctx.h>
#include <physics/springBone.h>
#include <core/nodeRegistry.h>



void Avatar::update(GlobalContext& ctx, float dt) {
	// --- 移動 ---
	float fx = -lutsv.getSin(yaw);
	float fz =  lutsv.getCos(yaw);

	float rx = -fz;   // -cos(yaw)
	float rz = -fx;   // = sin(yaw)

	bool walking = false;

	float rlMove = 0.7;
	float aplSpeed = speed * dt;

	if (has(ctx.keyStat, KCode::W)) {
		walking = true;
		pos.x -= fx * aplSpeed;
		pos.z += fz * aplSpeed;
	}
	if (has(ctx.keyStat, KCode::S)) {
		walking = true;
		pos.x += fx * aplSpeed;
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

	if ( has(ctx.keyStat, KCode::Space) ) {
		jump.jump(4);
	}
	jump.tick(dt);
	pos.y = jump.getHeight();

	status = (walking? Status::walk : Status::stay);

	// --- アニメーション ---
	// anim.tick(*this,dt);


	// --- 視点 ---
	if (has(ctx.keyStat, KCode::n0)) ctx.cam_type = CameraType::DEBUG;
	else if (has(ctx.keyStat, KCode::n1)) ctx.cam_type = CameraType::_1;

	const float yawDelta = ctx.mStat.relPos.x * sensitivity;
	head.yaw   -= yawDelta;
	head.pitch += ctx.mStat.relPos.y * sensitivity;

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
		yaw += head.yaw - clampedHeadYaw;
		head.yaw = clampedHeadYaw;
	}

	// --- neck (pitch) ---
	const NodeHandle& neckHandle = model.humanoid.bones[(size_t)HBT::neck];
	auto& neckNode = nodeReg.get(neckHandle);
	neckNode.mtx.setRot( Quat::fromAxisAngle({1,0,0}, head.pitch) );

	// --- head (yaw) ---
	const NodeHandle& headHandle = model.humanoid.bones[(size_t)HBT::head];
	auto& headNode = nodeReg.get(headHandle);
	headNode.mtx.setRot( Quat::fromAxisAngle({0,1,0}, head.yaw) );


	// update avatar mtx
	avatarMtx.setPos(pos);
	avatarMtx.setRot(Quat::fromAxisAngle({0,1,0}, yaw));
	avatarMtx.setScale(scale);
}


void Avatar::draw(Camera& cam) {

	NodeHandle headHandle = model.humanoid.bones[(size_t)HBT::head];

	int headIdx = nodeReg.getId(headHandle);
	if (headIdx < 0) return;

	// globalTransform のTRS成分は非一様スケール／鏡映を完全には表せないため、
	// 描画と同じグローバル行列から視点の位置・向きを取得する。
	const auto& headMtx = globalMtx[headIdx];
	vec3f lookDir = bx::mulXyz0({0.0f, 0.0f, 1.0f}, headMtx.data());
	lookDir.normalize();


	vec3f camPos = headMtx.pos()
		+ lookDir * 0.1f
		+ vec3f{0, 0.05f, 0};

	cam.update(camPos, camPos + lookDir);
}

Avatar::Avatar(const AvatarId& id): id(id), speed(3) {
	const fs::path base = syspath.userdata / "avatars" / id.hex();
	GltfType type = GltfType::binary;
	std::string path = (base / "model.vrm").string();

	GltfLoader loader(path, type);
	loader.load();
	VRM vrm = VRMLoader(loader);
	loader.procHdl(this);
	model = loader.get();
	model.init(vrm);

	visited.resize(model.nodes.size(), 0);

	// anim.init("test.sxa",*this);

}

Avatar::~Avatar() {
	for (const NodeHandle& hdl: model.nodeHandles) {
		nodeReg.destroy(hdl);
	}
}
