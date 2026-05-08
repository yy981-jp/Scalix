#pragma once

#include "def.h"
#include "../../shared/format.h"

Format convertUnityAnim(const json& ori);
Yaml parseUnityYaml(const std::string& path);
