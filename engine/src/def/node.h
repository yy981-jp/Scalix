#pragma once
#include <cstdint>
#include <functional>


using NodeId = int32_t; // -1(root)のためにsigned (avatar内一意)
using NodeEntryId = uint32_t; // NodeRegistry内のentryを指すId
using NodeGen = uint16_t;

struct NodeHandle {
	NodeEntryId id;
	NodeGen gen;

	auto operator<=>(const NodeHandle&) const = default; 
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