#include <gfx/shader.h>

#include <stdexcept>
#include <string>


bgfx::ShaderHandle loadShader(const std::string& path) {
	if (!fs::exists(path)) throw std::runtime_error(std::string{"Shader not found: "} + path);

	FILE* f = fopen(path.c_str(), "rb");
	if (!f) throw std::runtime_error("fopen failed");
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (size == 0) throw std::runtime_error("shader: size = 0");

	const bgfx::Memory* mem = bgfx::alloc(size);
	fread(mem->data, 1, size, f);
	fclose(f);

	return bgfx::createShader(mem);
}

bgfx::ProgramHandle loadProgram(const std::string& vs, const std::string& fs) {
	auto vsh = loadShader(vs);
	auto fsh = loadShader(fs);
	return bgfx::createProgram(vsh, fsh, true);
}

fs::path getRuntimeShaderDirectory(const fs::path& resourceDirectory) {
	const fs::path runtimeDirectory = resourceDirectory / "runtime";

#if defined(__linux__)
	switch (bgfx::getRendererType()) {
	case bgfx::RendererType::OpenGL:
		return runtimeDirectory / "opengl";
	case bgfx::RendererType::Vulkan:
		return runtimeDirectory / "vulkan";
	default:
		throw std::runtime_error(
			"No runtime shader variant is available for the selected Linux bgfx renderer ("
			+ std::to_string(static_cast<int>(bgfx::getRendererType())) + ")."
		);
	}
#else
	return runtimeDirectory;
#endif
}
