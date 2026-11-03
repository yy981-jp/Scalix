#pragma once
#include <def/json.h>
#include <def/vrm.h>

#include <def/fs.h>
#include <loader/gltf.h>


class VRMLoader {


public:
	VRMLoader(const json& extensions) {
		VRMC_VRM_1_0::Vrm vrm;
		VRMC_VRM_1_0::from_json(extensions.at("VRMC_vrm"), vrm);
	}
};
