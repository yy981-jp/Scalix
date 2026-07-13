#include <def/str.h>

#include <stdexcept>


StrHs StrTable::entry(std::string_view str) {
	StrHs hs = StrHs(str);
	table[hs] = str;
	return hs;
}

std::string_view StrTable::get(StrHs hash) {
	const auto pos = table.find(hash);
	if (pos == table.end())
		throw std::runtime_error("StrTable::get: hash not found");
	return pos->second;
}
