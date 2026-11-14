#include <loader/vrm.h>
#include <loader/gltf.h>



struct VRMLoaderImpl {
	VRM loaded;

	std::unordered_map<ModelFeatureFlag, std::string> featureNameMap;

	const GltfLoader& gltfloader;

	json getExt(const ModelFeature& target) {
		const std::string& name = featureNameMap.at(
			static_cast<ModelFeatureFlag>(target)
		);
		return gltfloader.getExtension(name);
	}


	template<typename T>
	void loadCore(ModelFeature target, T& out) {
		if (!has(flag, target)) return;

		const std::string& name = featureNameMap.at(
			static_cast<ModelFeatureFlag>(target)
		);
	}

	void loadSpringBone() {
		if (!has(flag, ModelFeature::VRMC_springBone)) return;

		const std::string& name = featureNameMap.at(
			static_cast<ModelFeatureFlag>(ModelFeature::VRMC_springBone)
		);
		const json extension = gltfloader.getExtension(name);
		auto res_opt = vrm10::parseSpringBone(getExt(ModelFeature::VRMC_springBone));
		if (!res_opt.hasValue()) throw std::runtime_error("tmp"); // TODO: fix message
		const auto& res = res_opt.value();
		loaded.springCenters.reserve(res.springs.size());

		for (const auto& spring: res.springs) {
			if (spring.center) {
				loaded.springCenters.emplace_back(spring.center->value);
			} else {
				loaded.springCenters.emplace_back(std::nullopt);
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

		vrm10::parseVrm(getExt(ModelFeature::VRMC_vrm));
		loadSpringBone();
	}
};


VRM VRMLoader(const GltfLoader& gltfloader) {
	VRMLoaderImpl impl(gltfloader);
	return impl.loaded;
}
