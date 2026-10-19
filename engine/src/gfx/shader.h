#pragma once
#include <def/fs.h>

#include <bgfx/bgfx.h>

bgfx::ShaderHandle loadShader(const std::string& path);
bgfx::ProgramHandle loadProgram(const std::string& vs, const std::string& fs);
