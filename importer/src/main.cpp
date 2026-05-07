#include <iostream>
#include <fstream>
#include <limits>
#include <filesystem>

#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>

#include "../../shared/format.h"


using json = nlohmann::json;
using Yaml = YAML::Node;
namespace fs = std::filesystem;

void dev_checkPattern() {
	// throw std::runtime_error("dev_checkPattern");
}


bool hasTwoOrMore(const std::string& str, const std::string& target) {
    size_t pos = 0;
    int count = 0;

    while ((pos = str.find(target, pos)) != std::string::npos) {
        count++;
        if (count >= 2) return true;
        pos += target.length(); // 次の位置へ
    }

    return false;
}

void removeUnity(std::string& file) {
	size_t break1 = file.find("\n");
	size_t break2 = file.find("\n", break1 + 1);
	size_t break3 = file.find("\n", break2 + 1);
	file.erase(0,break3+1);
}


json yamlToJson(const Yaml& node) {
	if(node.IsScalar()) {
		try { return node.as<int>(); } catch(...) {}
		try { return node.as<double>(); } catch(...) {}
		try { return node.as<bool>(); } catch(...) {}
		return node.as<std::string>();
	}
	else if(node.IsSequence()) {
		json j = json::array();
		for(const auto& it : node) {
			j.push_back(yamlToJson(it));
		}
		return j;
	}
	else if(node.IsMap()) {
		json j;
		for(const auto& it : node) {
			j[it.first.as<std::string>()] = yamlToJson(it.second);
		}
		return j;
	}
	return {};
}

Yaml parseUnityYaml(const std::string& path) {
	std::ifstream ifs(path);
	if (!ifs) throw std::runtime_error("ifs");
	std::string file{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
	if (hasTwoOrMore(file,"---")) throw std::runtime_error("unknown anim file");
	removeUnity(file);
	return YAML::Load(file);
}

Format::Key::Value parseValue(const json& v) {
	if (v.is_boolean()) return v.get<bool>();
	if (v.is_number()) return v.get<float>();
	// if (v.is_array())  return v.get<Format::Quat>();
	if (v.is_object()) {
		// printf("object-size: %d", v.size());
		switch (v.size()) {
			case 3: return Format::vec3f{
				v["x"].get<float>(),
				v["y"].get<float>(),
				v["z"].get<float>()
			};
		}
	}
	if (v.is_string() && (v.get<std::string>() == "Infinity")) return std::numeric_limits<float>::infinity();
	std::cout << v.dump(4) << "\n";
	throw std::runtime_error("invalid value");
}

Format convertUnityAnim(const json& ori) {
	const json& j = ori["AnimationClip"];
	Format f;
	f.sampleRate = j["m_SampleRate"];
	f.name = j["m_Name"];

	static const std::unordered_map<std::string, Format::Proc> map = {
		{"m_RotationCurves", Format::Proc::rotation},
		{"m_PositionCurves", Format::Proc::position},
		{"m_ScaleCurves", Format::Proc::scale},
		{"m_FloatCurves", Format::Proc::float_},
	};

	// tracks
	for (const auto& [key,path]: map) {
		// curve配列単位
		for (const json& value: j[key]) {
			// curve単位
			Format::Track f_track;

			f_track.proc = path;

			if (!value["path"].is_null()) // nullはrootであり、正常値
				f_track.target = value["path"];

			switch (path) {
				using enum Format::Proc;
				
				case float_: {
					for (const json& tracks: value["curve"]["m_Curve"]) {
						Format::Key key;
						key.time = tracks["time"];
						key.value = parseValue(tracks["value"]);
						f_track.keys.push_back(key);
					}
					f_track.interpolation = Format::Interpolation::liner; // TODO: 一旦これで... いいんちゃう?多分
					f_track.type = Format::Type::float_;
					std::string blendShape = value["attribute"];
					if (blendShape.starts_with("blendShape.")) {
						blendShape.erase(0,11); // "blendShape."を除去
						f_track.proc = Format::Proc::morph;
					} else f_track.proc = Format::Proc::active;
					f_track.attrTarget = blendShape;
				} break;

				case scale: {
					for (const json& tracks: value["curve"]["m_Curve"]) {
						Format::Key key;
						key.time = tracks["time"];
						key.value = parseValue(tracks["value"]);
						f_track.keys.push_back(key);
					}
					f_track.interpolation = Format::Interpolation::liner; // TODO: 一旦これで... いいんちゃう?多分
					f_track.type = Format::Type::vec3f;
				}

				default: dev_checkPattern();
			}
			f.tracks.push_back(f_track);
		}
	}
	return f;
}


json run(const std::string& path) {
	Yaml yml = parseUnityYaml(path);
	json j = yamlToJson(yml);
	Format fm = convertUnityAnim(j);
	return fm;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cout << "Usage: <targetDir>\n";
		return 1;
	}
	fs::path targetDir = argv[1];
	// fs::path currentDir = fs::current_path();

	// fs::create_directory("sxim_o");

	// std::cout << run("Sweater_OFF.anim").dump(4);

	json res;
	res["version"] = 1;
	res["body"] = json::array();

	for (const auto& e: fs::recursive_directory_iterator(targetDir)) {
		if (e.path().extension() != ".anim" ) continue;
		const fs::path& f = e.path().string();
		// fs::path rel = fs::relative(f,targetDir);
		//  currentDir / "sxim_o" / rel
		res["body"].push_back(run(f.string()));
	}

	std::ofstream ofs("test.sxa");
	ofs << res;
}
