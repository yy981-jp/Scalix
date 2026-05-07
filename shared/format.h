#pragma once


#include <string>
#include <array>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>


using json = nlohmann::json;


struct Format {
	enum class Type {
		unknown,
		quat,		// float x 4
		vec3f,		// float x 3
		float_		// float x 1
	};

	enum class Interpolation {
		unknown, liner, step, hermite/*点で傾き+速度で制御*/,
	};

	using Quat = std::array<float, 4>;
	using vec3f = std::array<float, 3>;

	struct Key {
		using Value = std::variant<float, vec3f, Quat, bool>;
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

void from_json(const json& j, Format::vec3f& v);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Format::Track,
	target, proc, type, interpolation, attrTarget, keys)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Format,
	sampleRate, name, tracks)
