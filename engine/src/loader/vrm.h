#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <def/json.h>
#include <def/vrm.h>
#include <def/fs.h>


using ModelFeatureFlag = uint8_t;

enum class ModelFeature: ModelFeatureFlag {
	None,
	KHR_materials_unlit,
	VRMC_materials_mtoon,
	VRMC_node_constraint,
	VRMC_springBone,
	VRMC_vrm,

	Count
};

#define X(name) {#name, ModelFeature::name},
const std::unordered_map<std::string_view, ModelFeature> modelFeatureMap {
	X(KHR_materials_unlit)
	X(VRMC_materials_mtoon)
	X(VRMC_node_constraint)
	X(VRMC_springBone)
	X(VRMC_vrm)
};
#undef X

inline bool has(ModelFeatureFlag flag, ModelFeature target) {
	return flag & 1u << static_cast<ModelFeatureFlag>(target);
}

struct VRM {
	VRMC_VRM_1_0::Vrm vrm;
	VRMC_VRM_1_0::SpringBone springBone;

	// VRM.h stores an omitted Spring::center as index 0. Preserve whether
	// the field was actually present so node 0 remains a valid center.
	std::vector<std::optional<uint32_t>> springCenters;
};


struct VRMLoaderImpl;
class GltfLoader;

VRM VRMLoader(const GltfLoader& gltfloader);
