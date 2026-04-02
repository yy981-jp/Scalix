#include <stdexcept>

#include <tinygltf/tiny_gltf.h>

#include "loader.h"


void GltfLoaderImpl::parse() {
	for (const auto& n : model.nodes) {
		if (n.mesh < 0) continue;

		// ===== NODE =====
		Node node;
		node.meshIndex = n.mesh;

		// translation
		if (!n.translation.empty()) {
			node.hasTranslation = true;
			node.pos[0] = (float)n.translation[0];
			node.pos[1] = (float)n.translation[1];
			node.pos[2] = (float)n.translation[2];
		}

		// rotation
		if (!n.rotation.empty()) {
			node.hasRotation = true;
			// glTFクォータニオン (x, y, z, w) の共役を取る: (x, y, z, w) -> (-x, -y, -z, w)
			node.rot[0] = -(float)n.rotation[0];
			node.rot[1] = -(float)n.rotation[1];
			node.rot[2] = -(float)n.rotation[2];
			node.rot[3] = (float)n.rotation[3];
		}

		// scale
		if (!n.scale.empty()) {
			node.hasScale = true;
			node.scale[0] = (float)n.scale[0];
			node.scale[1] = (float)n.scale[1];
			node.scale[2] = (float)n.scale[2];
		}

		scalixModel.nodes.push_back(node);


		const auto& mesh = model.meshes[n.mesh];

		for (const auto& prim : mesh.primitives) {
			// TRIANGLESのみ対応
			if (prim.mode != TINYGLTF_MODE_TRIANGLES)
				throw std::runtime_error("only TRIANGLES supported");

			auto& attrs = prim.attributes;

			// 毎回vertexデータをクリア
			v.clear();

			// ===== POSITION =====
			if (!attrs.count("POSITION"))
				throw std::runtime_error("no POSITION");

			posAcc = model.accessors[attrs.at("POSITION")];
			posView = model.bufferViews[posAcc.bufferView];
			posBuf = model.buffers[posView.buffer];

			posStride = posAcc.ByteStride(posView);
			if (posStride == 0) posStride = sizeof(float) * 3;

			posPtr = posBuf.data.data() + posView.byteOffset + posAcc.byteOffset;

			// ===== Normal/UV クリア =====
			norPtr = nullptr;
			norStride = 0;
			uvPtr = nullptr;
			uvStride = 0;

			// ===== NORMAL（任意）=====
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

			// ===== index =====
			std::vector<uint16_t> idx;

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

					uint16_t index = 0;

					switch (ia.componentType) {
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
						throw std::runtime_error("8bit index is unsupported now");
						index = *reinterpret_cast<const uint8_t*>(ptr);
						break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
						index = *reinterpret_cast<const uint16_t*>(ptr);
						break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
						throw std::runtime_error("32bit index is unsupported now");
						index = *reinterpret_cast<const uint16_t*>(ptr);
						break;
					default:
						throw std::runtime_error("unsupported index type");
					}

					idx.push_back(index);
				}

			} else {
				// indexなし → 連番
				for (uint16_t i = 0; i < posAcc.count; i++)
					idx.push_back(i);
			}

			// ===== 頂点生成 =====
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


			if (v.empty())
				throw std::runtime_error("vertex empty");

			Mesh mesh;
			mesh.create(v, idx);
			scalixModel.meshes.push_back(mesh);
		}
	}
}


GltfLoaderImpl::GltfLoaderImpl(const std::string& path): path(path) {}


void GltfLoaderImpl::load() {
	tinygltf::TinyGLTF loader;

	std::string err, warn;
	if (!loader.LoadASCIIFromFile(&model, &err, &warn, path)) {
		throw std::runtime_error("gltf load failed: " + err);
	}

	if (model.meshes.empty())
		throw std::runtime_error("no mesh");

	const auto& mesh = model.meshes[0];
	if (mesh.primitives.empty())
		throw std::runtime_error("no primitive");

	parse();

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
			Texture tex;
			tex.create(img.width, img.height, 4, rgba.data(), rgba.size());
			scalixModel.textures.push_back(tex);
		}
		else {
			Texture tex;
			tex.create(img.width, img.height, 4, img.image.data(), img.image.size());
			scalixModel.textures.push_back(tex);
		}
	}
}
