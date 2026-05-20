#pragma once
#include "../anim/format.h"
#include "../core/json.h"

#include <unordered_map>


struct Avater;

std::unordered_map<StrHs,AnimRtFmt> loadAnim(const std::string& path, const Avater& avater);
