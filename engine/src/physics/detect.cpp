#include <physics/detect.h>

#include <core/avatar.h>


std::vector<NodeHandle> detectBonePath(NodeHandle begin, NodeHandle end) {
	std::vector<NodeHandle> path;

	if (!nodeReg.is_alive(begin) || !nodeReg.is_alive(end))
		return path;

	Avatar* avatar = nodeReg.getAvatar(begin);
	if (avatar != nodeReg.getAvatar(end))
		return path;

	const std::vector<Node>& nodes = avatar->model.nodes;
	const NodeId beginId = nodeReg.getId(begin);
	NodeId cur = nodeReg.getId(end);
	std::vector<bool> visited(nodes.size(), false);
	bool foundBegin = false;

	while (cur != -1) {
		if (cur < 0 || static_cast<size_t>(cur) >= nodes.size() || visited[cur])
			return {};
		visited[cur] = true;

		NodeHandle handle = nodeReg.find(avatar->id, cur);
		if (!nodeReg.is_alive(handle))
			return {};
		path.push_back(handle);

		if (cur == beginId) {
			foundBegin = true;
			break;
		}

		cur = nodes[cur].parent;
	}

	if (!foundBegin)
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

		cur = nodeReg.find(avatarId, node.children[0]);
	}

	return path;
}
