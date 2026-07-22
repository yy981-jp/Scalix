#pragma once
#include <cstdint>
#include <functional>


struct NodeId {
    uint32_t value;

    constexpr NodeId(): value(invalid()) {};
    constexpr NodeId(uint32_t v): value(v) {}

    constexpr operator uint32_t() const { return value; }

    auto operator<=>(const NodeId&) const = default;

	template <std::integral T>
	constexpr auto operator<=>(T other) const {
		return value <=> static_cast<uint32_t>(other);
	}
	constexpr NodeId& operator++() {
		++value;
		return *this;
	}

	constexpr NodeId operator++(int) {
		NodeId tmp = *this;
		++*this;
		return tmp;
	}
	static constexpr NodeId invalid() { return UINT32_MAX; }
	
	constexpr bool isValid() const { return *this != invalid(); }
};

using NodeEntryId = uint32_t; // NodeRegistry内のentryを指すId
using NodeGen = uint16_t;

struct NodeHandle {
	NodeEntryId id;
	NodeGen gen;

	auto operator<=>(const NodeHandle&) const = default;
	static constexpr NodeHandle invalid() { return {UINT32_MAX,UINT16_MAX}; }
	constexpr bool isValid() const { return *this != invalid(); }
};


template<>
struct std::hash<NodeHandle> {
    size_t operator()(const NodeHandle& h) const noexcept {
        size_t h1 = std::hash<NodeEntryId>{}(h.id);
        size_t h2 = std::hash<NodeGen>{}(h.gen);

        // boost::hash_combine風
        return h1 ^ (h2 + 0x9e3779b97f4a7c15ull + (h1 << 6) + (h1 >> 2));
    }
};