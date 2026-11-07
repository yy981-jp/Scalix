#include <loader/vrm.h>
#include <loader/gltf.h>



struct VRMLoaderImpl {
	VRM res;

	std::unordered_map<ModelFeatureFlag, std::string> featureNameMap;

	const GltfLoader& gltfloader;

	template<typename T>
	void loadCore(ModelFeature target, T& out) {
		if (!has(flag, target)) return;

		const std::string& name = featureNameMap.at(
			static_cast<ModelFeatureFlag>(target)
		);
		VRMC_VRM_1_0::from_json(gltfloader.getExtension(name), out);
	}

	void loadSpringBone() {
		if (!has(flag, ModelFeature::VRMC_springBone)) return;

		const std::string& name = featureNameMap.at(
			static_cast<ModelFeatureFlag>(ModelFeature::VRMC_springBone)
		);
		const json extension = gltfloader.getExtension(name);
		VRMC_VRM_1_0::from_json(extension, res.springBone);

		const auto springs = extension.find("springs");
		if (springs == extension.end() || !springs->is_array()) return;

		res.springCenters.reserve(springs->size());
		for (const auto& spring: *springs) {
			const auto center = spring.find("center");
			if (center == spring.end() || center->is_null()) {
				res.springCenters.emplace_back(std::nullopt);
			} else {
				res.springCenters.emplace_back(center->get<uint32_t>());
			}
		}
	}

	ModelFeatureFlag flag = 0;

	VRMLoaderImpl(const GltfLoader& gltfloader): gltfloader(gltfloader) {
		for (const auto& ext: gltfloader.getExtensionsUsed()) {
			const auto it = modelFeatureMap.find(ext);
			if (it == modelFeatureMap.end()) continue;

			const ModelFeature feature = it->second;
			const auto featureId = static_cast<ModelFeatureFlag>(feature);
			flag |= static_cast<ModelFeatureFlag>(1u << featureId);
			featureNameMap[featureId] = ext;
		}

		loadCore(ModelFeature::VRMC_vrm, res.vrm);
		loadSpringBone();
	}
};


VRM VRMLoader(const GltfLoader& gltfloader) {
	VRMLoaderImpl impl(gltfloader);
	return impl.res;
}
