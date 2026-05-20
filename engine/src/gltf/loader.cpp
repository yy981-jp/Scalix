#include <stdexcept>

#include <tinygltf/tiny_gltf.h>
#include <bx/math.h>

#include "loader.h"
#include <set>
#include <cassert>
#include <iostream>


void GltfLoaderImpl::parseMesh(int nodeId) {
	const auto& tn = model.nodes[nodeId];
	const auto& tnmesh = model.meshes[tn.mesh];

	for (const auto& prim : tnmesh.primitives) {
		Mesh mesh;

		// TRIANGLESのみ対応
		if (prim.mode != TINYGLTF_MODE_TRIANGLES)
			throw std::runtime_error("only TRIANGLES supported");

		auto& attrs = prim.attributes;

		std::vector<Vertex> verts;

		// ===== POSITION =====
		if (!attrs.count("POSITION"))
			throw std::runtime_error("no POSITION");

		posAcc = model.accessors[attrs.at("POSITION")];
		posView = model.bufferViews[posAcc.bufferView];
		posBuf = model.buffers[posView.buffer];

		size_t posStride = posAcc.ByteStride(posView);
		if (posStride == 0) posStride = sizeof(float) * 3;

		const uint8_t* posPtr = posBuf.data.data() + posView.byteOffset + posAcc.byteOffset;

		// ===== Normal/UV クリア =====
		const float* norPtr = nullptr;
		size_t norStride = 0;
		const float* uvPtr = nullptr;
		size_t uvStride = 0;

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
		verts.reserve(posAcc.count);

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

			verts.push_back(vert);
		}

		if (verts.empty())
			throw std::runtime_error("vertex empty");

		// primitiveのマテリアルをmeshに設定
		if (prim.material >= 0) {
			mesh.materialIndex = prim.material;
		}


		// ===== JOINTS =====
		if (attrs.count("JOINTS_0")) {
			const auto& acc = model.accessors[attrs.at("JOINTS_0")];
			const auto& view = model.bufferViews[acc.bufferView];
			const auto& buf = model.buffers[view.buffer];

			const uint8_t* data = buf.data.data() + view.byteOffset + acc.byteOffset;

			size_t stride = acc.ByteStride(view);
			if (stride == 0) {
				if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
					stride = 4;
				else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					stride = 8;
			}

			for (size_t i = 0; i < acc.count; i++) {
				auto& vert = verts[i];
				const uint8_t* ptr = data + stride * i;

				if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
					for (int k = 0; k < 4; k++)
						vert.joints[k] = ptr[k];

				} else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
					const uint16_t* j = reinterpret_cast<const uint16_t*>(ptr);
					for (int k = 0; k < 4; k++)
						vert.joints[k] = j[k];
				}

				// if (i == 0) {
				// 	printf("joint: %d %d %d %d\n",
				// 		vert.joints[0],
				// 		vert.joints[1],
				// 		vert.joints[2],
				// 		vert.joints[3]);
				// }
			}
		}

		// ===== WEIGHTS =====
		const auto& acc = model.accessors[attrs.at("WEIGHTS_0")];
		const auto& view = model.bufferViews[acc.bufferView];
		const auto& buf = model.buffers[view.buffer];

		const uint8_t* data = buf.data.data() + view.byteOffset + acc.byteOffset;

		size_t stride = acc.ByteStride(view);

		// printf("componentType: %d normalized: %d stride: %zu\n",
		// 	acc.componentType,
		// 	acc.normalized,
		// 	stride
		// );

		if (stride == 0) {
			switch (acc.componentType) {
				case TINYGLTF_COMPONENT_TYPE_FLOAT: stride = sizeof(float)*4; break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: stride = 4; break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: stride = 8; break;
			}
		}

		for (size_t i = 0; i < acc.count; i++) {
			auto& vert = verts[i];
			const uint8_t* ptr = data + stride * i;
			// printf("raw: %d %d %d %d\n", ptr[0], ptr[1], ptr[2], ptr[3]);

			switch (acc.componentType) {
				case TINYGLTF_COMPONENT_TYPE_FLOAT: {
					const float* w = reinterpret_cast<const float*>(ptr);
					for (int k = 0; k < 4; k++) vert.weights[k] = w[k];
				} break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
					const uint8_t* w = ptr;
					for (int k = 0; k < 4; k++) vert.weights[k] = w[k] / 255.0f;
				} break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
					const uint16_t* w = reinterpret_cast<const uint16_t*>(ptr);
					for (int k = 0; k < 4; k++) vert.weights[k] = w[k] / 65535.0f;
				} break;
			}
		}

		mesh.verts = verts;

		// Mesh全体格納
		mesh.create(verts, idx);
		scalixModel.meshes.push_back(mesh);

	}
}


void GltfLoaderImpl::parse() {
	int nodesSize = model.nodes.size();
	scalixModel.nodes.resize(nodesSize);
	for (int i = 0; i < nodesSize; i++) {
		// ===== NODE =====
		const auto& tn = model.nodes[i];
		Node& node = scalixModel.nodes[i];
		node.skinIndex = tn.skin;
		node.meshStartIndex = scalixModel.meshes.size();  // 現在のメッシュ数を開始インデックスとして記録

		// translation
		if (!tn.translation.empty()) {
			node.hasTranslation = true;
			node.pos.x = (float)tn.translation[0];
			node.pos.y = (float)tn.translation[1];
			node.pos.z = (float)tn.translation[2];
		}

		// rotation
		if (!tn.rotation.empty()) {
			node.hasRotation = true;
			// glTFクォータニオン (x, y, z, w) の共役を取る: (x, y, z, w) -> (-x, -y, -z, w)
			node.rot[0] = -(float)tn.rotation[0];
			node.rot[1] = -(float)tn.rotation[1];
			node.rot[2] = -(float)tn.rotation[2];
			node.rot[3] = (float)tn.rotation[3];
		}

		// scale
		if (!tn.scale.empty()) {
			node.hasScale = true;
			node.scale[0] = (float)tn.scale[0];
			node.scale[1] = (float)tn.scale[1];
			node.scale[2] = (float)tn.scale[2];
		}

		// meshが存在するnodeに対してのみ実行
		if (tn.mesh >= 0) parseMesh(i);

		// ノードのメッシュ数を設定
		node.meshCount = scalixModel.meshes.size() - node.meshStartIndex;

		// node parent children 登録
		node.parent = -1; // 初期値
		for (const auto& child: tn.children) {
			if (child < 0 || child >= nodesSize) {
				printf("invalid child index: %d\n", child);
				continue;
			}

			if (scalixModel.nodes[child].parent != -1) {
				printf("multiple parent: %d\n", child);
			}

			scalixModel.nodes[child].parent = i;
		}
		node.children = tn.children;

	}

	// load skin (bone)
	for (const auto& model_skin : model.skins) {
		Skin skin;
		// joint (=bone)
		// printf("model_skin.joints.size: %d\n", model_skin.joints.size());
		for (int joint: model_skin.joints) {
			skin.joints.push_back(joint);
			scalixModel.nodes[joint].name = strsv().entry( model.nodes[joint].name );
			// printf("joint: %d\n", joint);
		}
		// skin
		if (model_skin.inverseBindMatrices >= 0) {
			// glTFからskin::invBindを読み込み
			const auto& accessor = model.accessors[model_skin.inverseBindMatrices];
			const auto& bv = model.bufferViews[accessor.bufferView];
			const auto& buf = model.buffers[bv.buffer];

			const float* data = reinterpret_cast<const float*>(
				buf.data.data() + bv.byteOffset + accessor.byteOffset
			);

			skin.invBind.resize(accessor.count);

			for (size_t i = 0; i < accessor.count; i++) {
				const float* src = data + i * 16;

				memcpy(skin.invBind[i].data(), src, sizeof(float) * 16);
			}

		} else {
			// glTFにデータが無い場合はidentityで初期化
			for (size_t i = 0; i < skin.joints.size(); i++) {
				for (auto& m : scalixModel.skins[i].invBind) {
					bx::mtxIdentity(m.data());
				}
			}
		}
		scalixModel.skins.push_back(skin);

		// for (int i = 0; i < skin.joints.size(); i++) {
		// 	printf("joint[%d] = node %d\n", i, skin.joints[i]);
		// }
		// for (int i = 0; i < skin.invBind.size(); i++) {
		// 	printf("invBind[%d] loaded\n", i);
		// }
	}
}

void GltfLoaderImpl::buildPalletCompress() {
	// ===== pallet圧縮 =====

	for (const auto& node : scalixModel.nodes) {

		if (node.skinIndex < 0) continue;

		const auto& skin = scalixModel.skins[node.skinIndex];
		int totalBoneCount = skin.joints.size();

		std::vector<int> remap(totalBoneCount, -1);
		std::vector<int> remapInverse;

		int newIndex = 0;

		// このnodeに紐づくmeshだけ処理する必要あり
		for (int mi = 0; mi < node.meshCount; mi++) {
			auto& mesh = scalixModel.meshes[node.meshStartIndex + mi];

					
		for (auto& vert : mesh.verts) {

			for (int i = 0; i < 4; i++) {
				if (vert.weights[i] <= 0.0001f) continue;

				int nodeIndex = vert.joints[i]; // ← ここが本質

				// nodeIndex → jointIndex変換
				int jointIndex = -1;
				for (int j = 0; j < skin.joints.size(); j++) {
					if (skin.joints[j] == nodeIndex) {
						jointIndex = j;
						break;
					}
				}

				if (jointIndex == -1) continue;

				int orig = jointIndex;

				if (remap[orig] == -1) {
					remap[orig] = newIndex;
					remapInverse.push_back(orig);
					newIndex++;
				}
			}
		}


			mesh.boneRemap = remap;
			mesh.boneRemapInverse = remapInverse;
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
		if (texIdx < 0 || texIdx >= static_cast<int>(model.textures.size())) {
			printf("Warning: Material[%zu] invalid texture index %d\n", matIdx, texIdx);
			continue;
		}
		
		int imgIdx = model.textures[texIdx].source;
		if (imgIdx < 0 || imgIdx >= static_cast<int>(model.images.size())) {
			printf("Warning: Material[%zu] texture invalid image index %d\n", matIdx, imgIdx);
			continue;
		}
		
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

			// テクスチャスロットをイメージインデックスに合わせる
			if (imgIdx >= static_cast<int>(scalixModel.textures.size())) {
				scalixModel.textures.resize(imgIdx + 1);
			}
			scalixModel.textures[imgIdx] = tex;
			// printf("Texture[%d] created successfully\n", imgIdx);
		} catch (const std::exception& e) {
			printf("Error loading image[%d]: %s\n", imgIdx, e.what());
		}
	}

	buildPalletCompress();
}
