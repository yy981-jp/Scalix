#include <loader/vrm.h>
#include <loader/gltf.h>



struct VRMLoaderImpl {
	VRM res;

	std::unordered_map<ModelFeatureFlag, std::string> featureNameMap;

	const GltfLoader& gltfloader;

	template<typename T>
	void loadCore(ModelFeature target, T& out) {
		if (has(flag,target)) {
			const json& name = featureNameMap.at(static_cast<ModelFeatureFlag>(target));
			VRMC_VRM_1_0::from_json(gltfloader.getExtension(name), out);
		}
	}

	ModelFeatureFlag flag;



	VRMLoaderImpl(const GltfLoader& gltfloader): gltfloader(gltfloader) {
		// build flag
		
		for (const auto& ext: gltfloader.getExtensionsUsed()) {
			const auto& extFlag = modelFeatureMap.at(ext);
			flag |= 1u << static_cast<ModelFeatureFlag>(extFlag);
			featureNameMap[static_cast<ModelFeatureFlag>(extFlag)] = ext;
		}

		loadCore(ModelFeature::VRMC_vrm, res.vrm);
		loadCore(ModelFeature::VRMC_springBone, res.springBone);

	}
};


VRM VRMLoader(const GltfLoader& gltfloader) {
	VRMLoaderImpl impl(gltfloader);
	return impl.res;
}
