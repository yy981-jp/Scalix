/**
 * @brief String Id
 */


#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <compare>
#include <unordered_map>


constexpr uint64_t fnv1a(std::string_view s) {
	uint64_t hash = 14695981039346656037ull;

	for (unsigned char c : s) {
		hash ^= c;
		hash *= 1099511628211ull;
	}

	return hash;
}


struct StrHs {
	uint64_t hash{};

	constexpr StrHs() = default;

	explicit constexpr StrHs(uint64_t i)
		: hash(i) {}

	explicit constexpr StrHs(std::string_view s)
		: hash(fnv1a(s)) {}

	auto operator<=>(const StrHs&) const = default;
};


constexpr StrHs operator ""_hs(const char* str, size_t len) {
	return StrHs{fnv1a(std::string_view(str, len))};
}


namespace std {

template<>
struct hash<StrHs> {
	size_t operator()(const StrHs& s) const noexcept {
		return s.hash;
	}
};

}




class StrTable {
	std::unordered_map<StrHs,std::string> table;

public:
	StrHs entry(std::string_view str) {
		StrHs hs = StrHs(str);
		table[hs] = str;
		return hs;
	}

	std::string_view get(StrHs hash) {
		const auto& pos = table.find(hash);
		if (pos != table.end()) return pos->second;
			else return {};
	}
};


// string table server ...関数名はそのうち改名しても良いかもね
constexpr StrTable& strsv() {
	static StrTable table;
	return table;
}
