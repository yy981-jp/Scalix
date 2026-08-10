#include "def/node.h"
#include "util/math.h"
#include <tracker/pose.h>
#include <tracker/poseSolver.h>

#include <debug/debugDraw.h>

#include <core/nodeRegistry.h>
#include <util/fmutil.h>
#include <def/str.h>


struct LandmarkConnection {
	LandmarkId a;
	LandmarkId b;
};


struct HumanoidConnection {
	HBT bone;
	LandmarkId parent;
	LandmarkId child;
};

enum class HMCnsId {
	upperBody,
	lowerBody,

	Count
};

const std::vector<HumanoidConnection> upperBody_list = {
	HumanoidConnection{HBT::leftUpperArm, LandmarkId::LeftShoulder, LandmarkId::LeftElbow},
	HumanoidConnection{HBT::leftLowerArm, LandmarkId::LeftElbow, LandmarkId::LeftWrist},
	// HumanoidConnection{HBT::rightUpperArm, LandmarkId::RightShoulder, LandmarkId::RightElbow},
	// HumanoidConnection{HBT::rightLowerArm, LandmarkId::RightElbow, LandmarkId::RightWrist},
};

const std::vector<HumanoidConnection> lowerBody_list = {
	HumanoidConnection{HBT::leftUpperLeg, LandmarkId::LeftHip, LandmarkId::LeftKnee},
	HumanoidConnection{HBT::leftLowerLeg, LandmarkId::LeftKnee, LandmarkId::LeftAnkle},
	HumanoidConnection{HBT::rightUpperLeg, LandmarkId::RightHip, LandmarkId::RightKnee},
	HumanoidConnection{HBT::rightLowerLeg, LandmarkId::RightKnee, LandmarkId::RightAnkle},
};

const std::vector<std::vector<HumanoidConnection>> humanoidConnections = {
	upperBody_list,
	lowerBody_list,
};


constexpr LandmarkConnection poseConnections[] = {
    // 顔（目）
    {LandmarkId::Nose, LandmarkId::LeftEyeInner},
    {LandmarkId::LeftEyeInner, LandmarkId::LeftEye},
    {LandmarkId::LeftEye, LandmarkId::LeftEyeOuter},
    {LandmarkId::Nose, LandmarkId::RightEyeInner},
    {LandmarkId::RightEyeInner, LandmarkId::RightEye},
    {LandmarkId::RightEye, LandmarkId::RightEyeOuter},

    // 顔（耳・口）
    {LandmarkId::LeftEar, LandmarkId::MouthLeft},
    {LandmarkId::RightEar, LandmarkId::MouthRight},
    {LandmarkId::MouthLeft, LandmarkId::MouthRight},

    // 上半身（両肩）
    {LandmarkId::LeftShoulder, LandmarkId::RightShoulder},

    // 腕・手（左）
    {LandmarkId::LeftShoulder, LandmarkId::LeftElbow},
    {LandmarkId::LeftElbow, LandmarkId::LeftWrist},
    {LandmarkId::LeftWrist, LandmarkId::LeftPinky},
    {LandmarkId::LeftWrist, LandmarkId::LeftIndex},
    {LandmarkId::LeftWrist, LandmarkId::LeftThumb},
    {LandmarkId::LeftPinky, LandmarkId::LeftIndex},

    // 腕・手（右）
    {LandmarkId::RightShoulder, LandmarkId::RightElbow},
    {LandmarkId::RightElbow, LandmarkId::RightWrist},
    {LandmarkId::RightWrist, LandmarkId::RightPinky},
    {LandmarkId::RightWrist, LandmarkId::RightIndex},
    {LandmarkId::RightWrist, LandmarkId::RightThumb},
    {LandmarkId::RightPinky, LandmarkId::RightIndex},

    // 胴体（肩〜腰）
    {LandmarkId::LeftShoulder, LandmarkId::LeftHip},
    {LandmarkId::RightShoulder, LandmarkId::RightHip},
    {LandmarkId::LeftHip, LandmarkId::RightHip},

    // 脚・足（左）
    {LandmarkId::LeftHip, LandmarkId::LeftKnee},
    {LandmarkId::LeftKnee, LandmarkId::LeftAnkle},
    {LandmarkId::LeftAnkle, LandmarkId::LeftHeel},
    {LandmarkId::LeftHeel, LandmarkId::LeftFootIndex},
    {LandmarkId::LeftAnkle, LandmarkId::LeftFootIndex},

    // 脚・足（右）
    {LandmarkId::RightHip, LandmarkId::RightKnee},
    {LandmarkId::RightKnee, LandmarkId::RightAnkle},
    {LandmarkId::RightAnkle, LandmarkId::RightHeel},
    {LandmarkId::RightHeel, LandmarkId::RightFootIndex},
    {LandmarkId::RightAnkle, LandmarkId::RightFootIndex},
};


// Humanoid::parentTable は「親」しか引けないので、逆引き用のヘルパー。
// 腕のチェーン(leftUpperArm -> leftLowerArm -> leftHand 等)では子は一意に決まる。
HBT childBoneOf(HBT bone) {
	for (size_t i = 0; i < (size_t)HBT::Count; i++) {
		if (Humanoid::parentTable[i] == bone) return (HBT)i;
	}
	return HBT::Count;
}


PoseSolver::PoseSolver(Avatar& avatar): avatar(avatar) {
	for (const auto& cnct : upperBody_list) {
		NodeHandle& nodeHdl = avatar.humanoid.bones[(size_t)cnct.bone];

		// restDir は node.trs.rot（このボーン自身の回転）ではなく、
		// 「このボーンの根本の関節 -> 次の関節(子ボーンの原点)」の向きでなければならない。
		// 例えば leftUpperArm なら「肩関節 -> 肘関節」。
		// 以前は Humanoid::parentTable(=leftShoulder、鎖骨側)との差分を使っていたため、
		// 実質「鎖骨の向き」を restDir にしてしまっており、
		// solve() 側の currentDir(肩ランドマーク -> 肘ランドマーク)と
		// そもそも比較対象がズレていた。
		HBT childBone = childBoneOf(cnct.bone);
		NodeHandle& childHdl = avatar.humanoid.bones[(size_t)childBone];

		bones[(size_t)cnct.bone].restDir =
			( avatar.trackingTransforms[nodeReg.getId(childHdl)].pos
			- avatar.trackingTransforms[nodeReg.getId(nodeHdl)].pos
			).normalized();

		// このボーン自身のバインドポーズでのグローバル回転をそのまま保存する。
		// trackingTransforms は calcGlobal() で親→子の順にローカルTRSを合成した
		// 結果なので、.rot にはこのボーンまでの回転がすべて含まれている。
		bones[(size_t)cnct.bone].globalBindRot =
			avatar.trackingTransforms[nodeReg.getId(nodeHdl)].rot;

		// 子ノードの「ローカル」位置オフセット(= このボーンの局所空間内での
		// 子ボーンの向き)。実際のスキニングパイプラインが使う値そのもの。
		bones[(size_t)cnct.bone].childLocalDir =
			nodeReg.get(childHdl).trs.pos.normalized();
	}


	// for (const auto& cnct: upperBody_list) {
	// 	const Node& node = nodeReg.get( avatar.humanoid.bones[(size_t)cnct.bone] );
	// 	const Mtx& invBind = avatar.model.skins[ node.skinIndex ].invBind[node.jointIndex];
	// 	Mtx bind = Mtx::inverse(invBind);
	// }
}


void PoseSolver::solve(const PoseFrame& frame) {
    debug_(frame);

	// このフレームで新たに計算したボーンのグローバル回転(トラッキング空間)を
	// キャッシュしておく。leftLowerArm の親は leftUpperArm であり、
	// upperBody_list は親→子の順で並んでいるため、子を解く時点では
	// avatar.trackingTransforms はまだ「前フレーム」の値のまま(このsolve()の後で
	// avatarSystem.update()が再計算する)。stale な値を親の回転として使うと
	// 特に手首側でズレ・破綻が起きるため、直前に計算した値を優先して使う。
	std::array<Quat, static_cast<size_t>(HBT::Count)> solvedGlobalRot{};
	std::array<bool, static_cast<size_t>(HBT::Count)> hasSolvedGlobalRot{};

	for (const auto& cnct: upperBody_list) {
		// const auto& cnct = upperBody_list[0];

		const vec3f& parent =
			frame.landmarks[(size_t)cnct.parent].pos;
		const vec3f& child =
			frame.landmarks[(size_t)cnct.child].pos;

		const vec3f& currentDir = (child - parent).normalized();
		const auto& bone = bones[(size_t)cnct.bone];

		Node& node =
			nodeReg.get(avatar.humanoid.bones[(size_t)cnct.bone]);

		HBT parentBone = Humanoid::parentTable[(size_t)cnct.bone];
		NodeHandle parentHdl = avatar.humanoid.bones[(size_t)parentBone];

		Quat parentGlobalRot =
			hasSolvedGlobalRot[(size_t)parentBone]
				? solvedGlobalRot[(size_t)parentBone]
				: avatar.trackingTransforms[nodeReg.getId(parentHdl)].rot;

		// restDir/currentDir はどちらもワールド(トラッキング)空間のベクトルなので、
		// fromTo() で得られる差分回転もワールド空間の回転になる。
		// これを bone.globalBindRot(バインドポーズでのこのボーンのグローバル回転)に
		// 適用すれば、このボーンの「現在のグローバル回転」が直接求まる。
		Quat worldDelta = Quat::fromTo(bone.restDir, currentDir);
		Quat newGlobalRot = worldDelta * bone.globalBindRot;

		// node.trs.rot は親ボーンのローカル空間での回転なので、
		// 親ボーンの「現在の」グローバル回転を割って(逆回転をかけて)ローカルに変換する。
		// ※ここで使う parentGlobalRot は必ず「現在」の値でなければならない。
		// leftLowerArm のように親(leftUpperArm)自身もトラッキングで動く場合、
		// bind時の親回転と現在の親回転は別物になるため、
		// 親の回転で挟み込む(サンドイッチする)方式だと式が成立しない。
		Quat parentGlobalRotInv = Quat::inverse(parentGlobalRot);

		node.trs.rot = parentGlobalRotInv * newGlobalRot;

		solvedGlobalRot[(size_t)cnct.bone] = newGlobalRot;
		hasSolvedGlobalRot[(size_t)cnct.bone] = true;




		// ===== Debug Draw =====

		vec3f origin = parent + vec3f{0, 2, 0};

		{
			// bind = inverse(invBind)
			const Node& node =
				nodeReg.get(avatar.humanoid.bones[(size_t)cnct.bone]);

			const Mtx& invBind =
				avatar.model.skins[node.skinIndex].invBind[node.jointIndex];

			Mtx bind = Mtx::inverse(invBind);

			const float* m = bind.data();

			// // column-major
			// vec3f axisX = { m[0], m[1], m[2] };
			// vec3f axisY = { m[4], m[5], m[6] };
			// vec3f axisZ = { m[8], m[9], m[10] };

			// axisX.normalize();
			// axisY.normalize();
			// axisZ.normalize();

			// debug.drawLine(origin, origin + axisX * 0.3f, 0xff0000ff); // X = 赤
			// debug.drawLine(origin, origin + axisY * 0.3f, 0xff00ff00); // Y = 緑
			// debug.drawLine(origin, origin + axisZ * 0.3f, 0xffff0000); // Z = 青

			// MediaPipe方向(白)
			// debug.drawLine(origin, origin + currentDir * 0.3f, 0xffffffff);
		}

		vec3f parentPos =
			avatar.trackingTransforms[nodeReg.getId(parentHdl)].pos;

		vec3f childPos =
			avatar.trackingTransforms[node.id].pos;

		vec3f debug_restDir =
			(childPos - parentPos).normalized();

		debug.drawLine(
			origin,
			origin + (solvedGlobalRot[(size_t)cnct.bone] * bone.childLocalDir) * 0.3f,
			0xff00ffff // 黄色
		);
		// puts("==============================");
		
		// const auto& test = node.trs.rot;
		// printf("restDir: %.3f %.3f %.3f %.3f\n"
		// 		, test.w, test.x, test.y, test.z
		// );
	}
}


void PoseSolver::debug_(const PoseFrame& frame) {
	for (auto& lm : frame.landmarks) {
		if (lm.visibility < 0.5) continue;
		debug.drawCross(lm.pos + vec3f{0,2,0}, 0.01f, 0xffff0000u);
	}

	for (const auto& c : poseConnections) {
		debug.drawLine(
			frame.landmarks[static_cast<size_t>(c.a)].pos + vec3f{0,2,0},
			frame.landmarks[static_cast<size_t>(c.b)].pos + vec3f{0,2,0}
		);
	}
}
