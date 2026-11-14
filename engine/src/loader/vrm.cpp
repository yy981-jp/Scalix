#include <loader/vrm.h>
#include <loader/data.h>
#include <loader/gltf.h>



struct VRMLoaderImpl {
	LoadData ld;

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

		// printf("%s", getExt(ModelFeature::VRMC_springBone).dump(4).c_str());
		auto result = vrm10::parseSpringBone(getExt(ModelFeature::VRMC_springBone));
		if (!result.hasValue()) throw std::runtime_error(
			std::format("Failed to parse VRMC_springBone: {} in \"{}\"",
				result.error().message,result.error().path)
		);
		const auto& res = result.value();
		ld.vrm.springBone = res;


		ld.springCenters.reserve(res.springs.size());

		for (const auto& spring: res.springs) {
			if (spring.center) {
				ld.springCenters.emplace_back(spring.center->value);
			} else {
				ld.springCenters.emplace_back(std::nullopt);
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

		{
			auto result = vrm10::parseVrm(getExt(ModelFeature::VRMC_vrm));
			if (!result.hasValue()) throw std::runtime_error(
			std::format("Failed to parse VRMC_vrm: {} in \"{}\"",
				result.error().message,result.error().path)
			);
			ld.vrm.vrm = result.value();
		}
		loadSpringBone();
	}
};


void VRMLoader(LoadData& ld, const GltfLoader& gltfloader) {
	VRMLoaderImpl impl(gltfloader);
	ld = std::move(impl.ld);
}
