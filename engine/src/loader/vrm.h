#pragma once
#include <def/json.h>
#include <def/vrm.h>

#include <def/fs.h>
#include <loader/gltf.h>


struct VRMLoader {
	VRMC_VRM_1_0::Vrm vrm;

	VRMLoader(const json& extensions) {
		VRMC_VRM_1_0::from_json(extensions, vrm);
	}
};
