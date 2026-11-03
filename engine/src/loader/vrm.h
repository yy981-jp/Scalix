#pragma once
#define USE_VRMC_VRM_1_0
#include <def/json.h>
#include <VRMC/VRM.h>

#include <def/fs.h>
#include <loader/gltf.h>


class VRMLoader {


public:
	VRMLoader(const json& extensions) {
		fs::path f{"model.vrm"};

		VRMC_VRM_1_0::Vrm vrm;
		VRMC_VRM_1_0::from_json(extensions["VRMC_vrm"], vrm);
		
		

	}
};
