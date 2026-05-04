#pragma once
#include <bgfx/bgfx.h>

bgfx::ShaderHandle loadShader(const char* path);
bgfx::ProgramHandle loadProgram(const char* vs, const char* fs);
