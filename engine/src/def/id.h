#pragma once
#include <cstdint>
#include <compare>
#include <concepts>


template<typename Tag>
struct Id {
	uint32_t value;

	constexpr Id(): value(invalid()) {};
	constexpr Id(uint32_t v): value(v) {}

	constexpr operator uint32_t() const { return value; }

	auto operator<=>(const Id&) const = default;

	template <std::integral T>
	constexpr auto operator<=>(T other) const {
		return value <=> static_cast<uint32_t>(other);
	}
	constexpr Id& operator++() {
		++value;
		return *this;
	}

	constexpr Id operator++(int) {
		Id tmp = *this;
		++*this;
		return tmp;
	}
	static constexpr Id invalid() { return UINT32_MAX; }
	
	constexpr bool isValid() const { return *this != invalid(); }
};
