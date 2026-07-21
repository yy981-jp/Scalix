#include <physics/detect.h>

#include <core/avatar.h>

#include <unordered_set>


std::vector<NodeHandle> detectBonePath(NodeHandle begin, NodeHandle end) {
	std::vector<NodeHandle> path;

	if (!nodeReg.is_alive(begin) || !nodeReg.is_alive(end))
		return path;

	if (nodeReg.getAvatar(begin) != nodeReg.getAvatar(end))
		return path;

	std::unordered_set<NodeHandle> visited;

	for (NodeHandle cur = end; nodeReg.is_alive(cur); ) {
		if (!visited.insert(cur).second)
			return {};

		path.push_back(cur);

		if (cur == begin)
			break;

		cur = nodeReg.get(cur).parent;
	}

	if (path.empty() || path.back() != begin)
		return {};

	std::reverse(path.begin(), path.end());
	return path;
}

std::vector<NodeHandle> detectBonePath(NodeHandle begin) {
	std::vector<NodeHandle> path;

	if (!nodeReg.is_alive(begin))
		return path;

	Avatar* avatar = nodeReg.getAvatar(begin);
	const NodeId avatarId = avatar->id;

	NodeHandle cur = begin;

	while (nodeReg.is_alive(cur)) {
		path.push_back(cur);

		const Node& node = nodeReg.get(cur);

		if (node.children.size() != 1)
			break;

		cur = node.children[0];
	}

	return path;
}
