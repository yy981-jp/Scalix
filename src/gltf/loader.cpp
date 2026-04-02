#include <stdexcept>

#include <tinygltf/tiny_gltf.h>

#include "loader.h"
#include <set>


void GltfLoaderImpl::parse() {
	for (const auto& n : model.nodes) {
		if (n.mesh < 0) continue;

		// ===== NODE =====
		Node node;
		node.meshStartIndex = scalixModel.meshes.size();  // 現在のメッシュ数を開始インデックスとして記録

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
			
			// primitiveのマテリアルをmeshに設定
			if (prim.material >= 0) {
				mesh.materialIndex = prim.material;
			}
			
			scalixModel.meshes.push_back(mesh);
		}
		
		// ノードのメッシュ数を設定
		node.meshCount = scalixModel.meshes.size() - node.meshStartIndex;
		scalixModel.nodes.push_back(node);
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

	// ===== テクスチャ読み込み =====
	// マテリアル → イメージのマップを作成
	scalixModel.materialToImage.resize(model.materials.size(), -1);
	
	for (size_t matIdx = 0; matIdx < model.materials.size(); matIdx++) {
		const auto& mat = model.materials[matIdx];
		int texIdx = mat.pbrMetallicRoughness.baseColorTexture.index;
		
		if (texIdx < 0) {
			continue;
		}
		
		// テクスチャ → イメージの参照を取得
		// if (texIdx < 0 || texIdx >= static_cast<int>(model.textures.size())) {
		// 	printf("Warning: Material[%zu] invalid texture index %d\n", matIdx, texIdx);
		// 	continue;
		// }
		
		int imgIdx = model.textures[texIdx].source;
		// if (imgIdx < 0 || imgIdx >= static_cast<int>(model.images.size())) {
		// 	printf("Warning: Material[%zu] texture invalid image index %d\n", matIdx, imgIdx);
		// 	continue;
		// }
		
		scalixModel.materialToImage[matIdx] = imgIdx;
		// printf("Material[%zu] -> Texture[%d] -> Image[%d]\n", matIdx, texIdx, imgIdx);
	}
	
	// ===== イメージから一意のテクスチャを読み込み =====
	std::set<int> uniqueImages;
	for (int idx : scalixModel.materialToImage) {
		if (idx >= 0) {
			uniqueImages.insert(idx);
		}
	}
	
	// テクスチャスロット = イメージインデックス
	for (int imgIdx : uniqueImages) {
		if (imgIdx < 0 || imgIdx >= static_cast<int>(model.images.size())) {
			continue;
		}

		const auto& img = model.images[imgIdx];
		// printf("Loading image[%d]: %dx%d, components=%d\n", imgIdx, img.width, img.height, img.component);

		try {
			Texture tex;
			
			if (img.component == 3) {
				tex.createFromRGB(img.width, img.height, img.image.data(), img.image.size());
			} else if (img.component == 4) {
				tex.createFromRGBA(img.width, img.height, img.image.data(), img.image.size());
			} else {
				printf("Warning: Unsupported image component count: %d\n", img.component);
				continue;
			}

			// テクスチャスロットをイメージインデックスに合わせます
			if (imgIdx >= static_cast<int>(scalixModel.textures.size())) {
				scalixModel.textures.resize(imgIdx + 1);
			}
			scalixModel.textures[imgIdx] = tex;
			// printf("Texture[%d] created successfully\n", imgIdx);
		} catch (const std::exception& e) {
			printf("Error loading image[%d]: %s\n", imgIdx, e.what());
		}
	}
}
