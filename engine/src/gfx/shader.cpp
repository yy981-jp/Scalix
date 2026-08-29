#include <gfx/shader.h>
#include <def/fs.h>


bgfx::ShaderHandle loadShader(const char* path) {
	if (!fs::exists(path)) throw std::runtime_error(std::string{"Shader not found: "} + path);

	FILE* f = fopen(path, "rb");
	if (!f) throw std::runtime_error("fopen failed");
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (size == 0) throw std::runtime_error("shader: size = 0");

	const bgfx::Memory* mem = bgfx::alloc(size/* + 1*/);
	fread(mem->data, 1, size, f);
	fclose(f);
	// mem->data[mem->size - 1] = '\0';

	return bgfx::createShader(mem);
}

bgfx::ProgramHandle loadProgram(const char* vs, const char* fs) {
	auto vsh = loadShader(vs);
	auto fsh = loadShader(fs);
	return bgfx::createProgram(vsh, fsh, true);
}
