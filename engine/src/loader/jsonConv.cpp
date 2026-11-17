#include <stdexcept>
#include <string>
#include <vector>
#include <loader/jsonConv.h>


nlohmann::json convertToJson(const tinygltf::Value& value) {
	using json = nlohmann::json;

	switch (value.Type()) {
	case tinygltf::NULL_TYPE:
		return nullptr;

	case tinygltf::BOOL_TYPE:
		return value.Get<bool>();

	case tinygltf::INT_TYPE:
		return value.Get<int>();

	case tinygltf::REAL_TYPE:
		return value.Get<double>();

	case tinygltf::STRING_TYPE:
		return value.Get<std::string>();

	case tinygltf::ARRAY_TYPE: {
		json result = json::array();

		for (const auto& element : value.Get<tinygltf::Value::Array>()) {
			result.push_back(convertToJson(element));
		}

		return result;
	}

	case tinygltf::OBJECT_TYPE: {
		json result = json::object();

		for (const auto& [key, element] : value.Get<tinygltf::Value::Object>()) {
			result[key] = convertToJson(element);
		}

		return result;
	}

	case tinygltf::BINARY_TYPE:
		return json::binary(value.Get<std::vector<unsigned char>>());

	default:
		throw std::runtime_error("Unsupported tinygltf::Value type");
	}
}
