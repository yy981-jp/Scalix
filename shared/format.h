#pragma once


#include <string>
#include <array>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>


using json = nlohmann::json;


struct Format {
	enum class Type {
		unknown, quat, float_
	};

	enum class Interpolation {
		unknown, liner, step
	};

	using Quat = std::array<float, 4>;

	struct Key {
		using Value = std::variant<float, Quat, bool>;
		float time;
		Value value = std::numeric_limits<float>::quiet_NaN();
	};
	
	enum class Proc {
		unknown, rotation, position, scale, float_, active, morph
	};

	/// @brief 1動作
	struct Track {
		std::string target; // 動作対象
		Proc proc = Proc::unknown; // 処理の種類
		Type type = Type::unknown; // 型
		Interpolation interpolation = Interpolation::unknown; // 補完タイプ
		std::string attrTarget; // 具体的な動作の対象 moph名だとか
		std::vector<Key> keys; // キーフレーム
	};



	int sampleRate;
	std::string name;
	std::vector<Track> tracks;
};


void to_json(json& j, const Format::Key& k);
void from_json(const json& j, Format::Key& k);


NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Format::Track,
	target, proc, type, interpolation, attrTarget, keys)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Format,
	sampleRate, name, tracks)
