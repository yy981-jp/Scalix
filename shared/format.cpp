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
	else if (v.is_array())
		k.value = v.get<Format::Quat>();
}
