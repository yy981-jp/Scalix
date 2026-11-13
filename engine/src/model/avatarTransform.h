

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
