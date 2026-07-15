#include <def/transform.h>
#include <util/mtxutil.h>


Transform::Transform() : pos{}, rot{}, scale{1.0f, 1.0f, 1.0f} {
	mtx.fill(0.0f);
	rebuildMatrix();
}

Transform Transform::operator*(const Transform& rhs) const {
	Transform out;

	// Scale
	out.scale = scale * rhs.scale;

	// Rotation
	out.rot = rot * rhs.rot;

	// Position
	out.pos = pos + rot * (scale * rhs.pos);

	// TRS の成分だけでは、非一様スケールや鏡映を含む親子合成で shear を
	// 表現できない。描画・スキニング用キャッシュは元の行列積をそのまま使う。
	// bx::mtxMul(result, a, b) は数学的には b * a を格納する。
	bx::mtxMul(out.mtx.data(), rhs.mtx.data(), mtx.data());

	return out;
}

void Transform::rebuildMatrix() {
	buildTRS(mtx.data(), pos, rot, scale);
}
