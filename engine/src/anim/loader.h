#pragma once
#include <anim/format.h>
#include <def/json.h>

#include <unordered_map>


struct Avatar;

std::unordered_map<StrHs,AnimRtFmt> loadAnim(const std::string& path, const Avatar& avatar);
