#include "unity.h"

#include <iostream>
#include <fstream>

#include "util.h"


void removeUnity(std::string& file) {
	size_t break1 = file.find("\n");
	size_t break2 = file.find("\n", break1 + 1);
	size_t break3 = file.find("\n", break2 + 1);
	file.erase(0,break3+1);
}


Yaml parseUnityYaml(const std::string& path) {
	std::ifstream ifs(path);
	if (!ifs) throw std::runtime_error("ifs");
	std::string file{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
	if (hasTwoOrMore(file,"---")) throw std::runtime_error("unknown anim file");
	removeUnity(file);
	return YAML::Load(file);
}

FormatDef::Key::Value parseValue(const json& v) {
	if (v.is_boolean()) return v.get<bool>();
	if (v.is_number()) return v.get<float>();
	// if (v.is_array())  return v.get<FormatDef::Quat>();
	if (v.is_object()) {
		// printf("object-size: %d", v.size());
		switch (v.size()) {
			case 3: return FormatDef::vec3f{
				v["x"].get<float>(),
				v["y"].get<float>(),
				v["z"].get<float>()
			};
			case 4: return FormatDef::Quat{
				v["x"].get<float>(),
				v["y"].get<float>(),
				v["z"].get<float>(),
				v["w"].get<float>()
			};
		}
	}
	if (v.is_string() && (v.get<std::string>() == "Infinity")) return std::numeric_limits<float>::infinity();
	std::cerr << v.dump(4) << "\n";
	throw std::runtime_error("invalid value");
}


void dumpJson(const json& j) { std::cout << j.dump(); }

ImAnimObj convertUnityAnim(const json& ori) {
	const json& j = ori["AnimationClip"];
	ImAnimObj res;
	res.first = j["m_Name"];
	
	ImAnimFmt& f = res.second;
	f.sampleRate = j["m_SampleRate"];

	static const std::unordered_map<std::string, FormatDef::Proc> map = {
		{"m_RotationCurves", FormatDef::Proc::rotation},
		{"m_PositionCurves", FormatDef::Proc::position},
		{"m_ScaleCurves", FormatDef::Proc::scale},
		{"m_FloatCurves", FormatDef::Proc::float_},
	};

	// tracks
	for (const auto& [key,proc]: map) {
		// curve配列単位
		for (const auto& value: j[key]) {
			// curve単位
			ImAnimFmt::Track f_track;

			f_track.proc = proc;

			if (!(
				value.contains("curve") &&
				value["curve"].contains("m_Curve")
			)) throw std::runtime_error("unknown anim format: curve");

			f_track.keys.reserve(value["curve"]["m_Curve"].size());

			if (!value["path"].is_null()) // nullはrootであり、正常値
				f_track.target = value["path"];

			size_t f_track_index = 0;
			for (const json& tracks: value["curve"]["m_Curve"]) {
				FormatDef::Key key;
				key.time = tracks["time"];
				key.value = parseValue(tracks["value"]);
				f_track.keys.push_back(std::move(key));
				f_track_index++;
			}

			f_track.interpolation = FormatDef::Interpolation::liner; // TODO: 一旦これで... いいんちゃう?多分


			switch (proc) {
				using enum FormatDef::Proc;
				
				case float_: {
					f_track.type = FormatDef::Type::float_;
					std::string blendShape = value["attribute"];
					if (blendShape.starts_with("blendShape.")) {
						blendShape.erase(0,11); // "blendShape."を除去
						f_track.proc = FormatDef::Proc::morph;
					} else f_track.proc = FormatDef::Proc::active;
					f_track.attrTarget = blendShape;
				} break;

				case scale: {
					f_track.type = FormatDef::Type::vec3f;
				} break;
				
				case position: {
					f_track.type = FormatDef::Type::vec3f;
				} break;
				
				case rotation: {
					f_track.type = FormatDef::Type::quat;
					std::cout << value.dump() << "\n";
				} break;

				default: dev_checkPattern();
			}
			f.tracks.push_back(std::move(f_track));
		}
	}
	return res;
}

ImAnimObj run_unity(const std::string& path) {
	Yaml yml = parseUnityYaml(path);
	json j = yamlToJson(yml);
	yml = {};
	return convertUnityAnim(j);
}
