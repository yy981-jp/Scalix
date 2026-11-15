#pragma once
#include <cstdint>
#include <string_view>
#include <unordered_map>

#include <def/json.h>
#include <def/fs.h>
#include <loader/def.h>


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

struct VRMLoaderImpl;
class GltfLoader;

void VRMLoader(LoadData& ld, const GltfLoader& gltfloader);
