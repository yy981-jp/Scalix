/**
	@file
	@brief loader/def.hの実体
*/


#include <loader/def.h>
#include <vrm10/vrm10.hpp>


struct LoadData {
	struct {
		vrm10::Vrm vrm;
		vrm10::SpringBone springBone;

	} vrm;
	std::vector<std::optional<uint32_t>> springCenters;

};
