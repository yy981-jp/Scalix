#pragma once
#include "../gfx/mesh.h"
#include "../gfx/texture.h"
#include <tinygltf/tiny_gltf.h>

namespace GltfLoader {

struct Model {
	Mesh mesh;
	Texture texture;
};

const float* getFloat(const tinygltf::Model& m, const tinygltf::Accessor& a);
Model load(const char* path);

}