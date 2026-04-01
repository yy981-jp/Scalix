#include <stdexcept>

#include <tinygltf/tiny_gltf.h>

#include "loader.h"


namespace GltfLoader {


const float* getFloat(const tinygltf::Model& m, const tinygltf::Accessor& a) {
	const auto& view = m.bufferViews[a.bufferView];
	const auto& buf  = m.buffers[view.buffer];

	return reinterpret_cast<const float*>(
		buf.data.data() + view.byteOffset + a.byteOffset
	);
}


struct PosColorVertex {
    float x, y, z;
    uint32_t abgr;

    static void init(bgfx::VertexLayout& layout) {
        layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();
    }
};


Model sampleLoad() {
	Model r;

	PosColorVertex vertices[] = {
		{-1,  1,  1, 0xff0000ff}, { 1,  1,  1, 0xff00ff00}, { -1, -1,  1, 0xffff0000},
		{ 1, -1,  1, 0xffffffff},
		{-1,  1, -1, 0xff00ffff}, { 1,  1, -1, 0xffff00ff},
		{-1, -1, -1, 0xffffff00}, { 1, -1, -1, 0xff888888},
	};

	uint16_t indices[] = {
		0,1,2, 1,3,2,
		4,6,5, 5,6,7,
		0,2,4, 4,2,6,
		1,5,3, 5,7,3,
		0,4,1, 4,5,1,
		2,3,6, 6,3,7
	};

    bgfx::VertexLayout layout;
    PosColorVertex::init(layout);

    auto vbh = bgfx::createVertexBuffer(
        bgfx::copy(vertices, sizeof(vertices)),
        layout
    );

    auto ibh = bgfx::createIndexBuffer(
        bgfx::copy(indices, sizeof(indices)),
		BGFX_BUFFER_INDEX32
    );

	r.mesh.vbh = vbh;
	r.mesh.ibh = ibh;

	return r;
}


Model load(const char* path) {
	// return sampleLoad();

	tinygltf::Model model;
	tinygltf::TinyGLTF loader;

	std::string err, warn;
	if (!loader.LoadASCIIFromFile(&model, &err, &warn, path)) {
		throw std::runtime_error("gltf load failed: " + err);
	}

	if (model.meshes.empty())
		throw std::runtime_error("no mesh");

	Model r;

	const auto& mesh = model.meshes[0];
	if (mesh.primitives.empty())
		throw std::runtime_error("no primitive");

	const auto& prim = mesh.primitives[0];

	// TRIANGLESのみ対応
	if (prim.mode != TINYGLTF_MODE_TRIANGLES)
		throw std::runtime_error("only TRIANGLES supported");

	auto& attrs = prim.attributes;

	// ===== POSITION =====
	if (!attrs.count("POSITION"))
		throw std::runtime_error("no POSITION");

	const auto& posAcc = model.accessors[attrs.at("POSITION")];
	const auto& posView = model.bufferViews[posAcc.bufferView];
	const auto& posBuf = model.buffers[posView.buffer];

	size_t posStride = posAcc.ByteStride(posView);
	if (posStride == 0) posStride = sizeof(float) * 3;

	const uint8_t* posPtr = posBuf.data.data() + posView.byteOffset + posAcc.byteOffset;

	// ===== NORMAL（任意）=====
	const float* norPtr = nullptr;
	size_t norStride = 0;

	if (attrs.count("NORMAL")) {
		const auto& norAcc = model.accessors[attrs.at("NORMAL")];
		const auto& norView = model.bufferViews[norAcc.bufferView];
		const auto& norBuf = model.buffers[norView.buffer];

		norStride = norAcc.ByteStride(norView);
		if (norStride == 0) norStride = sizeof(float) * 3;

		norPtr = reinterpret_cast<const float*>(
			norBuf.data.data() + norView.byteOffset + norAcc.byteOffset
		);
	}

	// ===== UV（任意）=====
	const float* uvPtr = nullptr;
	size_t uvStride = 0;

	if (attrs.count("TEXCOORD_0")) {
		const auto& uvAcc = model.accessors[attrs.at("TEXCOORD_0")];
		const auto& uvView = model.bufferViews[uvAcc.bufferView];
		const auto& uvBuf = model.buffers[uvView.buffer];

		uvStride = uvAcc.ByteStride(uvView);
		if (uvStride == 0) uvStride = sizeof(float) * 2;

		uvPtr = reinterpret_cast<const float*>(
			uvBuf.data.data() + uvView.byteOffset + uvAcc.byteOffset
		);
	}

	// ===== 頂点生成 =====
	std::vector<Vertex> v;
	v.reserve(posAcc.count);

	for (size_t i = 0; i < posAcc.count; i++) {
		const float* p = reinterpret_cast<const float*>(posPtr + posStride * i);

		Vertex vert{};
		vert.x = p[0];
		vert.y = p[1];
		vert.z = p[2];

		if (norPtr) {
			const float* n = reinterpret_cast<const float*>(
				reinterpret_cast<const uint8_t*>(norPtr) + norStride * i
			);
			vert.nx = n[0];
			vert.ny = n[1];
			vert.nz = n[2];
		}

		if (uvPtr) {
			const float* u = reinterpret_cast<const float*>(
				reinterpret_cast<const uint8_t*>(uvPtr) + uvStride * i
			);
			vert.u = u[0];
			vert.v = u[1];
		}

		v.push_back(vert);
	}

	// ===== index =====
	std::vector<uint32_t> idx;

	if (prim.indices >= 0) {
		const auto& ia = model.accessors[prim.indices];
		const auto& iv = model.bufferViews[ia.bufferView];
		const auto& ib = model.buffers[iv.buffer];

		const uint8_t* data = ib.data.data() + iv.byteOffset + ia.byteOffset;

		size_t stride = ia.ByteStride(iv);
		if (stride == 0) {
			switch (ia.componentType) {
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  stride = 1; break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: stride = 2; break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   stride = 4; break;
			}
		}

		for (size_t i = 0; i < ia.count; i++) {
			const uint8_t* ptr = data + stride * i;

			uint32_t index = 0;

			switch (ia.componentType) {
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				index = *reinterpret_cast<const uint8_t*>(ptr);
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				index = *reinterpret_cast<const uint16_t*>(ptr);
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
				index = *reinterpret_cast<const uint32_t*>(ptr);
				break;
			default:
				throw std::runtime_error("unsupported index type");
			}

			idx.push_back(index);
		}

	} else {
		// indexなし → 連番
		for (uint32_t i = 0; i < posAcc.count; i++)
			idx.push_back(i);
	}

	r.mesh.create(v, idx);

	// ===== texture（超シンプル）=====
	if (!model.images.empty()) {
		const auto& img = model.images[0];

		int comp = img.component;
		if (comp != 4) {
			// RGB → RGBA変換
			std::vector<uint8_t> rgba(img.width * img.height * 4);
			for (int i = 0; i < img.width * img.height; i++) {
				rgba[i*4+0] = img.image[i*comp+0];
				rgba[i*4+1] = img.image[i*comp+1];
				rgba[i*4+2] = img.image[i*comp+2];
				rgba[i*4+3] = 255;
			}
			r.texture.create(img.width, img.height, 4, rgba.data(), rgba.size());
		}
		else {
			r.texture.create(img.width, img.height, 4, img.image.data(), img.image.size());
		}
	}

	return r;
}


}
