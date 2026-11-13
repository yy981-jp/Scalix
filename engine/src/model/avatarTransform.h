#pragma once
#include <model/avatarDef.h>
#include <core/avatar.h>
#include <def/mtx.h>


namespace AvatarTransform {
	void refreshAll(
		Avatar& avatar,
		const Mtx& entityMtx,
		std::vector<Mtx>& out
	);

	void refreshSubtree(
		Avatar& avatar,
		NodeHandle root,
		const Mtx& entityMtx,
		std::vector<Mtx>& out
	);
}
