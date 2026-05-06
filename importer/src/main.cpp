#include <fstream>

#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>


using json = nlohmann::json;
using Yaml = YAML::Node;


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
	file.find("\n");
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

std::vector<Yaml> parseUnityYaml(const std::string& path) {
	std::ifstream ifs(path);
	if (!ifs) throw std::runtime_error("ifs");
	std::string file;
	if (hasTwoOrMore(file,"---")) throw std::runtime_error("unknown anim file");
	removeUnity(file);
}


int main() {

}
