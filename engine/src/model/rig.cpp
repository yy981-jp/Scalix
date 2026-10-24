#include <model/rig.h>
#include <model/model.h>


void RigGraph::init(const Model& model) {
	size_t size = model.nodes.size();
	std::vector<bool> includeRig(size);
	std::vector<bool> explored(size);
	
	for (size_t i = 0; i < size; i++) {
		auto&& rig = includeRig[i];
		auto& node = model.nodes[i];

		
	}
}