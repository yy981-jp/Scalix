#include "format.h"


// ===== Format::Key =====

void to_json(json& j, const Format::Key& k) {
	j["time"] = k.time;

	std::visit([&](auto&& v) {
		j["value"] = v;
	}, k.value);
}

void from_json(const json& j, Format::Key& k) {
	k.time = j.at("time").get<float>();

	const auto& v = j.at("value");

	if (v.is_boolean())
		k.value = v.get<bool>();
	else if (v.is_number())
		k.value = v.get<float>();
	else if (v.is_array()) {
		if (v.size() == 3) k.value = v.get<Format::vec3f>();
		if (v.size() == 4) k.value = v.get<Format::Quat>();
	}
}

void from_json(const json& j, Format::vec3f& v) {
    v[0] = j.at("x").get<float>();
    v[1] = j.at("y").get<float>();
    v[2] = j.at("z").get<float>();
    // v[0] = j[0];
    // v[1] = j[1];
    // v[2] = j[2];
}

// void to_json(json& j, const Format::vec3f& v) {

// }
