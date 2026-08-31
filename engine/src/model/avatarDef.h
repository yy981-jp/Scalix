#pragma once
#include <cstdint>
// #include <compare>
#include <format>


struct UUID {
	uint64_t high;
	uint64_t low;

	UUID(const uint64_t& high, const uint64_t& low): high(high), low(low) {}
	UUID(): high(UINT64_MAX), low(UINT64_MAX) {}
	static UUID fromString(const std::string& str) {
		if (str.size() != 32) throw std::invalid_argument("Invalid UUID");
		
		UUID uuid;
		uuid.high = std::stoull(str.substr(0, 16), nullptr, 16);
		uuid.low = std::stoull(str.substr(16, 16), nullptr, 16);

		return uuid;
	}

	std::string hex() const { return std::format("{:016x}{:016x}", high, low); }

	auto operator<=>(const UUID&) const = default;
};

namespace std {
	template<>
	struct hash<UUID> {
		size_t operator()(const UUID& id) const noexcept {
			return static_cast<size_t>(id.high ^ id.low);
		}
	};
}

using AvatarId = UUID;

struct Avatar;
