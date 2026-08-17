#include <gltf/loader.h>

#include <tinygltf/tiny_gltf.h>
#include <bx/math.h>

#include <stdexcept>
#include <set>
#include <cassert>
#include <cstring>


namespace {

struct AccessorBytes {
	const tinygltf::Accessor* accessor = nullptr;
	const uint8_t* data = nullptr;
	size_t stride = 0;
	size_t count = 0;
	int componentType = 0;
	int type = 0;
};

size_t componentSize(int componentType) {
	switch (componentType) {
	case TINYGLTF_COMPONENT_TYPE_BYTE:
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		return 1;
	case TINYGLTF_COMPONENT_TYPE_SHORT:
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		return 2;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
	case TINYGLTF_COMPONENT_TYPE_FLOAT:
		return 4;
	default:
		throw std::runtime_error("unsupported accessor component type");
	}
}

size_t componentCount(int type) {
	switch (type) {
	case TINYGLTF_TYPE_SCALAR: return 1;
	case TINYGLTF_TYPE_VEC2:   return 2;
	case TINYGLTF_TYPE_VEC3:   return 3;
	case TINYGLTF_TYPE_VEC4:   return 4;
	case TINYGLTF_TYPE_MAT4:   return 16;
	default:
		throw std::runtime_error("unsupported accessor type");
	}
}

AccessorBytes getAccessorBytes(const tinygltf::Model& model, int accessorIndex) {
	if (accessorIndex < 0 || accessorIndex >= static_cast<int>(model.accessors.size()))
		throw std::runtime_error("invalid accessor index");

	const auto& acc = model.accessors[accessorIndex];
	if (acc.bufferView < 0 || acc.bufferView >= static_cast<int>(model.bufferViews.size()))
		throw std::runtime_error("invalid accessor bufferView");

	const auto& view = model.bufferViews[acc.bufferView];
	if (view.buffer < 0 || view.buffer >= static_cast<int>(model.buffers.size()))
		throw std::runtime_error("invalid accessor buffer");

	const auto& buf = model.buffers[view.buffer];

	AccessorBytes out;
	out.accessor = &acc;
	out.data = buf.data.data() + view.byteOffset + acc.byteOffset;
	out.stride = acc.ByteStride(view);
	if (out.stride == 0)
		out.stride = componentSize(acc.componentType) * componentCount(acc.type);
	out.count = acc.count;
	out.componentType = acc.componentType;
	out.type = acc.type;
	return out;
}

const uint8_t* accessorElement(const AccessorBytes& view, size_t index) {
	return view.data + view.stride * index;
}

const float* accessorFloatElement(const AccessorBytes& view, size_t index) {
	if (view.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
		throw std::runtime_error("expected float accessor");
	return reinterpret_cast<const float*>(accessorElement(view, index));
}

uint16_t readIndex(const AccessorBytes& view, size_t index) {
	const uint8_t* ptr = accessorElement(view, index);

	switch (view.componentType) {
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		throw std::runtime_error("8bit index is unsupported now");
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		return *reinterpret_cast<const uint16_t*>(ptr);
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		throw std::runtime_error("32bit index is unsupported now");
	default:
		throw std::runtime_error("unsupported index type");
	}
}

void readJoints(Vertex& vert, const uint8_t* ptr, int componentType) {
	switch (componentType) {
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		for (int k = 0; k < 4; k++)
			vert.joints[k] = ptr[k];
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
		const uint16_t* joints = reinterpret_cast<const uint16_t*>(ptr);
		for (int k = 0; k < 4; k++)
			vert.joints[k] = joints[k];
	} break;
	default:
		throw std::runtime_error("unsupported joint type");
	}
}

void readWeights(Vertex& vert, const uint8_t* ptr, int componentType) {
	switch (componentType) {
	case TINYGLTF_COMPONENT_TYPE_FLOAT: {
		const float* weights = reinterpret_cast<const float*>(ptr);
		for (int k = 0; k < 4; k++)
			vert.weights[k] = weights[k];
	} break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		for (int k = 0; k < 4; k++)
			vert.weights[k] = ptr[k] / 255.0f;
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
		const uint16_t* weights = reinterpret_cast<const uint16_t*>(ptr);
		for (int k = 0; k < 4; k++)
			vert.weights[k] = weights[k] / 65535.0f;
	} break;
	default:
		throw std::runtime_error("unsupported weight type");
	}
}

} // namespace


void GltfLoaderImpl::parseMesh(NodeId nodeId) {
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

		const AccessorBytes pos = getAccessorBytes(model, attrs.at("POSITION"));



		// ===== Normal/UV クリア =====
		AccessorBytes nor{};
		AccessorBytes uv{};
		bool hasNormal = false;
		bool hasUv = false;

		// ===== NORMAL（任意）=====
		if (attrs.contains("NORMAL")) {
			nor = getAccessorBytes(model, attrs.at("NORMAL"));


			hasNormal = true;
		}

		// ===== UV（任意）=====
		if (attrs.contains("TEXCOORD_0")) {
			uv = getAccessorBytes(model, attrs.at("TEXCOORD_0"));


			hasUv = true;
		}

		// ===== index =====
		std::vector<uint16_t> idx;

		if (prim.indices >= 0) {
			const auto& indexAcc = getAccessorBytes(model, prim.indices);
			for (size_t i = 0; i < indexAcc.count; i++) {
				idx.push_back(readIndex(indexAcc, i));
			}
		} else {
			// indexなし → 連番
			for (uint16_t i = 0; i < pos.count; i++)
				idx.push_back(i);
		}

		// ===== 頂点生成 =====
		verts.reserve(pos.count);

		for (size_t i = 0; i < pos.count; i++) {
			const float* p = accessorFloatElement(pos, i);

			Vertex vert{};
			vert.x = p[0];
			vert.y = p[1];
			vert.z = p[2];

			if (hasNormal) {
				const float* n = accessorFloatElement(nor, i);
				vert.nx = n[0];
				vert.ny = n[1];
				vert.nz = n[2];
			}

			if (hasUv) {
				const float* u = accessorFloatElement(uv, i);
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
		if (attrs.contains("JOINTS_0")) {
			const auto& jointAcc = getAccessorBytes(model, attrs.at("JOINTS_0"));
			for (size_t i = 0; i < jointAcc.count; i++) {
				readJoints(verts[i], accessorElement(jointAcc, i), jointAcc.componentType);
			}
		}

		// ===== WEIGHTS =====
		if (attrs.contains("WEIGHTS_0")) {
			const auto& weightAcc = getAccessorBytes(model, attrs.at("WEIGHTS_0"));
			for (size_t i = 0; i < weightAcc.count; i++) {
				readWeights(verts[i], accessorElement(weightAcc, i), weightAcc.componentType);
			}
		}

		mesh.verts = verts;
		mesh.indices = idx;

		// Mesh全体格納
		scalixModel.meshes.push_back(mesh);

	}
}


void GltfLoaderImpl::parse() {
	int nodesSize = model.nodes.size();
	scalixModel.nodes.resize(nodesSize);
	scalixModel.nodeHandles.resize(nodesSize);
	lnodes.resize(nodesSize);
	for (int i = 0; i < nodesSize; i++) {
		// ===== NODE =====
		const auto& tn = model.nodes[i];
		Node& node = scalixModel.nodes[i];
		LoadingNode& lnode = lnodes[i];
		node.skinIndex = tn.skin;
		node.meshStartIndex = scalixModel.meshes.size();  // 現在のメッシュ数を開始インデックスとして記録

		// translation
		if (!tn.translation.empty()) {
			node.trs.pos.x = (float)tn.translation[0];
			node.trs.pos.y = (float)tn.translation[1];
			node.trs.pos.z = (float)tn.translation[2];
		}

		// rotation
		if (!tn.rotation.empty()) {
			// glTFクォータニオン (x, y, z, w) の共役を取る: (x, y, z, w) -> (-x, -y, -z, w)
			node.trs.rot = {
				(float)tn.rotation[0],
				(float)tn.rotation[1],
				(float)tn.rotation[2],
				(float)tn.rotation[3]
			};
		}

		// scale
		if (!tn.scale.empty()) {
			node.trs.scale = {
				(float)tn.scale[0],
				(float)tn.scale[1],
				(float)tn.scale[2]
			};
		}

		// meshが存在するnodeに対してのみ実行
		if (tn.mesh >= 0) parseMesh(i);

		// ノードのメッシュ数を設定
		node.meshCount = scalixModel.meshes.size() - node.meshStartIndex;

		// node parent children 登録
		lnode.parent = NodeId::invalid(); // 初期値
		
		int children_size = tn.children.size();
		lnode.children.resize(children_size);
		for (int j = 0; j < children_size; j++) {
			const auto& child = tn.children[j];

			if (child < 0 || child >= nodesSize) {
				printf("invalid child index: %d\n", child);
				continue;
			}

			if (lnodes[child].parent != NodeId::invalid()) {
				printf("multiple parent: %d\n", child);
			}

			lnodes[child].parent = i;

			lnode.children[j] = tn.children[j];

		}
		lnode.id = i;
		node.name = strsv().entry(tn.name);
		
	}

	// load skin (bone)
	for (const auto& model_skin : model.skins) {
		Skin skin;
		// joint (=bone)
		// printf("model_skin.joints.size: %d\n", model_skin.joints.size());
		for (int joint: model_skin.joints) {
			skin.joints.push_back(joint);
			// printf("joint: %d\n", joint);
		}
		// skin
		if (model_skin.inverseBindMatrices >= 0) {
			// glTFからskin::invBindを読み込み
			const auto& invAcc = getAccessorBytes(model, model_skin.inverseBindMatrices);
			
			skin.invBind.resize(invAcc.count);
			for (size_t i = 0; i < invAcc.count; i++) {
				const float* src = accessorFloatElement(invAcc, i);
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

		for (size_t jointIndex = 0; jointIndex < skin.joints.size(); ++jointIndex) {
			int nodeIndex = skin.joints[jointIndex];
			if (nodeIndex >= 0 && nodeIndex < nodesSize) {
				auto& node = scalixModel.nodes[nodeIndex];
				node.skinIndex = static_cast<int>(scalixModel.skins.size());
				node.jointIndex = static_cast<int>(jointIndex);
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
		if (node.skinIndex >= static_cast<int>(scalixModel.skins.size()))
			throw std::runtime_error("invalid skin index");

		const auto& skin = scalixModel.skins[node.skinIndex];
		int totalBoneCount = skin.joints.size();

		// このnodeに紐づくmeshだけ処理する必要あり
		for (int mi = 0; mi < node.meshCount; mi++) {
			auto& mesh = scalixModel.meshes[node.meshStartIndex + mi];

			std::vector<int> remap(totalBoneCount, -1);
			std::vector<int> remapInverse;
			int newIndex = 0;

			for (auto& vert : mesh.verts) {

				for (int i = 0; i < 4; i++) {
					if (vert.weights[i] <= 0.0001f) continue;

					int orig = vert.joints[i];
					if (orig < 0 || orig >= totalBoneCount)
						throw std::runtime_error("invalid joint index");

					// Compact the original skin joint index into this mesh palette.
					if (remap[orig] == -1) {
						remap[orig] = newIndex;
						remapInverse.push_back(orig);
						newIndex++;
					}
					vert.joints[i] = static_cast<uint16_t>(remap[orig]);
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
			scalixModel.textures[imgIdx] = std::move(tex);
			// printf("Texture[%d] created successfully\n", imgIdx);
		} catch (const std::exception& e) {
			printf("Error loading image[%d]: %s\n", imgIdx, e.what());
		} catch (...) {
			throw std::runtime_error("unknown error: gltf/loader - image");
		}
	}

	buildPalletCompress();

	for (auto& mesh : scalixModel.meshes) {
		mesh.create(mesh.verts, mesh.indices);
	}
}

void GltfLoaderImpl::procHdl(Avatar* avatar) {
	// すべてのnodeを登録
	for (int i = 0; i < scalixModel.nodes.size(); i++) {
		LoadingNode& lnode = lnodes[i];
		Node& node = scalixModel.nodes[i];
		NodeHandle& nodeHdl = scalixModel.nodeHandles[i];
		
		// ハンドル取得
		const NodeHandle& handle = nodeReg.create(avatar,lnode.id);

		nodeHdl = handle;
		node.id = lnode.id;
	}

	// parentとchildrenもhdl解決
	for (int i = 0; i < scalixModel.nodes.size(); i++) {
		Node& node = scalixModel.nodes[i];
		LoadingNode& lnode = lnodes[i];

		// parent
		if (lnode.parent.isValid()) {
			// 通常
			node.parent = scalixModel.nodeHandles[lnode.parent];
		} else {
			// root
			node.parent = NodeHandle::invalid();
		}

		node.children.resize(lnode.children.size());
		for (int j = 0; j < lnode.children.size(); j++) {
			NodeId& lchild = lnode.children[j];
			NodeHandle& child = node.children[j];

			// child
			child = scalixModel.nodeHandles[lchild];
		}

	}

	handleSolved = true;
}
