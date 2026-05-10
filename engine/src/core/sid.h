#pragma once

#include <cstdint>
#include <string_view>
#include <compare>


constexpr uint64_t fnv1a(std::string_view s) {
	uint64_t hash = 14695981039346656037ull;

	for (unsigned char c : s) {
		hash ^= c;
		hash *= 1099511628211ull;
	}

	return hash;
}


struct StId {
	uint64_t hash{};

	constexpr StId() = default;

	explicit constexpr StId(uint64_t i)
		: hash(i) {}

	explicit constexpr StId(std::string_view s)
		: hash(fnv1a(s)) {}

	auto operator<=>(const StId&) const = default;
};


constexpr StId operator ""_hs(const char* str, size_t len) {
	return StId{fnv1a(std::string_view(str, len))};
}


namespace std {

template<>
struct hash<StId> {
	size_t operator()(const StId& s) const noexcept {
		return s.hash;
	}
};

}