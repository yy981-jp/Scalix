#pragma once


#include <string>
#include <array>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include <engine/src/def/str.h>
#include <engine/src/def/node.h>

using json = nlohmann::json;


namespace FormatDef {
	enum class Type {
		unknown,
		quat,		// float x 4
		vec3f,		// float x 3
		float_		// float x 1
	};

	enum class Interpolation {
		unknown,
		liner,		// 線形補完
		step,		// 補完無し
		hermite,	// 点で傾き+速度で制御
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
}

template<class StrT>
struct Format {
	/// @brief 1動作
	struct Track {
		StrT target; // 動作対象
		FormatDef::Proc proc = FormatDef::Proc::unknown; // 処理の種類
		FormatDef::Type type = FormatDef::Type::unknown; // 型
		FormatDef::Interpolation interpolation = FormatDef::Interpolation::unknown; // 補完タイプ
		StrT attrTarget; // 具体的な動作の対象 moph名だとか
		std::vector<FormatDef::Key> keys; // キーフレーム
	};

	int sampleRate;
	float stopTime;
	// StrT name;
	std::vector<Track> tracks;

	template<class OtherStrT>
	Format(const Format<OtherStrT>& src)
		: sampleRate(src.sampleRate), stopTime(src.stopTime) {
		// name = AnimFmt_convStr(src.name);

		tracks.reserve(src.tracks.size());

		for (const auto& t : src.tracks) {
			Track track;
			// track.target = AnimFmt_convStr(t.target);
			track.proc = t.proc;
			track.type = t.type;
			track.interpolation = t.interpolation;
			// track.attrTarget = AnimFmt_convStr(t.attrTarget);
			track.keys = t.keys;
			tracks.push_back(track);
		}
	}

	Format() = default;

};


// Forward declarations for custom JSON serialization
namespace nlohmann {
	template <>
	struct adl_serializer<FormatDef::Key> {
		static void to_json(json& j, const FormatDef::Key& k);
		static void from_json(const json& j, FormatDef::Key& k);
	};
}


using AnimImFmt = Format<std::string>;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AnimImFmt::Track,
	target, proc, type, interpolation, attrTarget, keys)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AnimImFmt,
	sampleRate, tracks)
