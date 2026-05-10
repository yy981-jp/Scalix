#include "format.h"


// ===== FormatDef::Key =====

namespace nlohmann {
	void adl_serializer<FormatDef::Key>::to_json(json& j, const FormatDef::Key& k) {
		j["time"] = k.time;

		std::visit([&](auto&& v) {
			j["value"] = v;
		}, k.value);
	}

	void adl_serializer<FormatDef::Key>::from_json(const json& j, FormatDef::Key& k) {
		k.time = j.at("time").get<float>();

		const auto& v = j.at("value");

		if (v.is_boolean())
			k.value = v.get<bool>();
		else if (v.is_number())
			k.value = v.get<float>();
		else if (v.is_array()) {
			if (v.size() == 3) k.value = v.get<FormatDef::vec3f>();
			if (v.size() == 4) k.value = v.get<FormatDef::Quat>();
		}
	}
}

void to_json(json& j, const FormatDef::Key& k) {
	nlohmann::adl_serializer<FormatDef::Key>::to_json(j, k);
}

void from_json(const json& j, FormatDef::Key& k) {
	nlohmann::adl_serializer<FormatDef::Key>::from_json(j, k);
}

void from_json(const json& j, FormatDef::vec3f& v) {
    v[0] = j.at("x").get<float>();
    v[1] = j.at("y").get<float>();
    v[2] = j.at("z").get<float>();
}
