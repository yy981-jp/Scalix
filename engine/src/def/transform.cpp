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
	out.rebuildMatrix();

	return out;
}

void Transform::rebuildMatrix() {
	buildTRS(mtx.data(), pos, rot, scale);
}
