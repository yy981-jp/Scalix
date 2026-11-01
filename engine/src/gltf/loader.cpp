#include <gltf/loader.h>

#include <tinygltf/tiny_gltf.h>
#include <bx/math.h>

#include <stdexcept>
#include <set>
#include <array>
#include <limits>
#include <cstring>


namespace {


struct AccessorBytes {
	std::vector<uint8_t> data;
	size_t stride = 0;
	size_t count = 0;
	int componentType = 0;
	int type = 0;
	bool normalized = false;
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
	case TINYGLTF_TYPE_VEC2: return 2;
	case TINYGLTF_TYPE_VEC3: return 3;
	case TINYGLTF_TYPE_VEC4: return 4;
	case TINYGLTF_TYPE_MAT2: return 4;
	case TINYGLTF_TYPE_MAT3: return 9;
	case TINYGLTF_TYPE_MAT4: return 16;
	default:
		throw std::runtime_error("unsupported accessor type");
	}
}

size_t matrixDimension(int type) {
	switch (type) {
	case TINYGLTF_TYPE_MAT2: return 2;
	case TINYGLTF_TYPE_MAT3: return 3;
	case TINYGLTF_TYPE_MAT4: return 4;
	default: return 0;
	}
}

size_t packedElementSize(int type, size_t componentBytes) {
	return componentCount(type) * componentBytes;
}

size_t storedElementSize(int type, size_t componentBytes) {
	const size_t dimension = matrixDimension(type);
	if (dimension == 0)
		return packedElementSize(type, componentBytes);

	const size_t columnBytes = dimension * componentBytes;
	const size_t alignedColumnBytes = (columnBytes + 3u) & ~size_t(3u);
	return dimension * alignedColumnBytes;
}

void copyAccessorElement(uint8_t* destination, const uint8_t* source,
	int type, size_t componentBytes) {
	const size_t dimension = matrixDimension(type);
	if (dimension == 0) {
		std::memcpy(destination, source, packedElementSize(type, componentBytes));
		return;
	}

	const size_t columnBytes = dimension * componentBytes;
	const size_t alignedColumnBytes = (columnBytes + 3u) & ~size_t(3u);
	for (size_t column = 0; column < dimension; ++column) {
		std::memcpy(
			destination + column * columnBytes,
			source + column * alignedColumnBytes,
			columnBytes
		);
	}
}

const tinygltf::BufferView& checkedBufferView(
	const tinygltf::Model& model, int viewIndex) {
	if (viewIndex < 0 ||
		static_cast<size_t>(viewIndex) >= model.bufferViews.size())
		throw std::runtime_error("invalid accessor bufferView index");

	const auto& view = model.bufferViews[static_cast<size_t>(viewIndex)];
	if (view.buffer < 0 ||
		static_cast<size_t>(view.buffer) >= model.buffers.size())
		throw std::runtime_error("invalid accessor buffer index");

	const auto& buffer = model.buffers[static_cast<size_t>(view.buffer)];
	if (view.byteOffset > buffer.data.size() ||
		view.byteLength > buffer.data.size() - view.byteOffset) {
		throw std::runtime_error("bufferView exceeds buffer bounds");
	}

	return view;
}

const uint8_t* bufferViewRange(
	const tinygltf::Model& model, int viewIndex, size_t offset, size_t length) {
	const auto& view = checkedBufferView(model, viewIndex);
	if (offset > view.byteLength || length > view.byteLength - offset)
		throw std::runtime_error("accessor data exceeds bufferView bounds");

	const auto& buffer = model.buffers[static_cast<size_t>(view.buffer)];
	return buffer.data.data() + view.byteOffset + offset;
}

uint32_t readUnsignedComponent(const uint8_t* ptr, int componentType) {
	switch (componentType) {
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		return ptr[0];
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
		uint16_t value{};
		std::memcpy(&value, ptr, sizeof(value));
		return value;
	}
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
		uint32_t value{};
		std::memcpy(&value, ptr, sizeof(value));
		return value;
	}
	default:
		throw std::runtime_error("unsupported unsigned accessor component type");
	}
}

AccessorBytes getAccessorBytes(const tinygltf::Model& model, int accessorIndex) {
	if (accessorIndex < 0 ||
		static_cast<size_t>(accessorIndex) >= model.accessors.size()) {
		throw std::runtime_error("invalid accessor index");
	}

	const auto& accessor = model.accessors[static_cast<size_t>(accessorIndex)];
	if (accessor.bufferView < -1)
		throw std::runtime_error("invalid accessor bufferView index");
	const size_t componentBytes = componentSize(accessor.componentType);
	const size_t elementBytes = packedElementSize(accessor.type, componentBytes);
	const size_t storedBytes = storedElementSize(accessor.type, componentBytes);

	if (accessor.count == 0)
		throw std::runtime_error("accessor count must be greater than zero");
	if (elementBytes == 0 ||
		accessor.count > std::numeric_limits<size_t>::max() / elementBytes) {
		throw std::runtime_error("accessor size overflow");
	}

	AccessorBytes out;
	out.stride = elementBytes;
	out.count = accessor.count;
	out.componentType = accessor.componentType;
	out.type = accessor.type;
	out.normalized = accessor.normalized;
	out.data.resize(accessor.count * elementBytes, 0);

	if (accessor.bufferView >= 0) {
		const auto& view = checkedBufferView(model, accessor.bufferView);
		if (accessor.byteOffset > view.byteLength ||
			storedBytes > view.byteLength - accessor.byteOffset) {
			throw std::runtime_error("accessor exceeds bufferView bounds");
		}
		if (accessor.byteOffset % componentBytes != 0 ||
			(view.byteOffset + accessor.byteOffset) % componentBytes != 0) {
			throw std::runtime_error("misaligned accessor byteOffset");
		}

		const size_t sourceStride = view.byteStride == 0 ? storedBytes : view.byteStride;
		if (sourceStride < storedBytes ||
			sourceStride % componentBytes != 0 ||
			(view.byteStride != 0 &&
				(sourceStride < 4u || sourceStride > 252u || sourceStride % 4u != 0))) {
			throw std::runtime_error("invalid accessor byteStride");
		}

		const size_t available = view.byteLength - accessor.byteOffset;
		if (accessor.count - 1 > (available - storedBytes) / sourceStride)
			throw std::runtime_error("accessor elements exceed bufferView bounds");

		const uint8_t* source = bufferViewRange(
			model, accessor.bufferView, accessor.byteOffset,
			(accessor.count - 1) * sourceStride + storedBytes
		);
		for (size_t i = 0; i < accessor.count; ++i) {
			copyAccessorElement(
				out.data.data() + i * elementBytes,
				source + i * sourceStride,
				accessor.type, componentBytes
			);
		}
	} else if (accessor.byteOffset != 0) {
		throw std::runtime_error("accessor without bufferView must have byteOffset zero");
	}

	// Sparse accessors replace selected elements in the base data. When no
	// bufferView exists, the base data above remains zero-initialized.
	if (accessor.sparse.isSparse) {
		const auto& sparse = accessor.sparse;
		if (sparse.count <= 0 ||
			static_cast<size_t>(sparse.count) > accessor.count) {
			throw std::runtime_error("invalid sparse accessor count");
		}

		const auto& indicesView = checkedBufferView(model, sparse.indices.bufferView);
		const auto& valuesView = checkedBufferView(model, sparse.values.bufferView);
		if (indicesView.byteStride != 0 || valuesView.byteStride != 0)
			throw std::runtime_error("sparse accessor bufferViews must not be strided");

		const size_t sparseCount = static_cast<size_t>(sparse.count);
		const size_t sparseIndexBytes = componentSize(sparse.indices.componentType);
		if (sparse.indices.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE &&
			sparse.indices.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT &&
			sparse.indices.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
			throw std::runtime_error("invalid sparse accessor index component type");
		}
		if (sparseCount > std::numeric_limits<size_t>::max() / sparseIndexBytes ||
			sparseCount > std::numeric_limits<size_t>::max() / storedBytes) {
			throw std::runtime_error("sparse accessor size overflow");
		}

		const uint8_t* sparseIndices = bufferViewRange(
			model, sparse.indices.bufferView, sparse.indices.byteOffset,
			sparseCount * sparseIndexBytes
		);
		const uint8_t* sparseValues = bufferViewRange(
			model, sparse.values.bufferView, sparse.values.byteOffset,
			sparseCount * storedBytes
		);
		if ((indicesView.byteOffset + sparse.indices.byteOffset) % sparseIndexBytes != 0 ||
			(valuesView.byteOffset + sparse.values.byteOffset) % componentBytes != 0) {
			throw std::runtime_error("misaligned sparse accessor data");
		}

		uint32_t previousIndex = 0;
		for (size_t i = 0; i < sparseCount; ++i) {
			const uint32_t index = readUnsignedComponent(
				sparseIndices + i * sparseIndexBytes,
				sparse.indices.componentType
			);
			if (index >= accessor.count || (i > 0 && index <= previousIndex))
				throw std::runtime_error("invalid sparse accessor index");
			previousIndex = index;

			copyAccessorElement(
				out.data.data() + static_cast<size_t>(index) * elementBytes,
				sparseValues + i * storedBytes,
				accessor.type, componentBytes
			);
		}
	}

	return out;
}

const uint8_t* accessorElement(const AccessorBytes& view, size_t index) {
	if (index >= view.count)
		throw std::runtime_error("accessor element index out of range");
	return view.data.data() + view.stride * index;
}

std::array<float, 16> accessorFloatElement(const AccessorBytes& view, size_t index) {
	if (view.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
		throw std::runtime_error("expected float accessor");
	const size_t count = componentCount(view.type);
	if (count > 16 || view.stride != count * sizeof(float))
		throw std::runtime_error("invalid float accessor element size");

	std::array<float, 16> result{};
	std::memcpy(result.data(), accessorElement(view, index), count * sizeof(float));
	return result;
}

uint32_t readIndex(const AccessorBytes& view, size_t index) {
	if (view.type != TINYGLTF_TYPE_SCALAR || view.normalized)
		throw std::runtime_error("indices accessor must be a non-normalized SCALAR");

	return readUnsignedComponent(accessorElement(view, index), view.componentType);
}

void readJoints(Vertex& vert, const uint8_t* ptr, int componentType) {
	switch (componentType) {
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		for (int k = 0; k < 4; k++)
			vert.joints[k] = ptr[k];
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		for (int k = 0; k < 4; k++) {
			uint16_t value{};
			std::memcpy(&value, ptr + k * sizeof(value), sizeof(value));
			vert.joints[k] = value;
		}
		break;
	default:
		throw std::runtime_error("unsupported joint type");
	}
}

void readWeights(Vertex& vert, const uint8_t* ptr, int componentType) {
	switch (componentType) {
	case TINYGLTF_COMPONENT_TYPE_FLOAT:
		for (int k = 0; k < 4; k++)
			std::memcpy(&vert.weights[k], ptr + k * sizeof(float), sizeof(float));
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		for (int k = 0; k < 4; k++)
			vert.weights[k] = ptr[k] / 255.0f;
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		for (int k = 0; k < 4; k++) {
			uint16_t value{};
			std::memcpy(&value, ptr + k * sizeof(value), sizeof(value));
			vert.weights[k] = value / 65535.0f;
		}
		break;
	default:
		throw std::runtime_error("unsupported weight type");
	}
}

} // namespace


void GltfLoader::parseMesh(NodeId nodeId) {
	const size_t nodeIndex = static_cast<uint32_t>(nodeId);
	if (nodeIndex >= model.nodes.size())
		throw std::runtime_error("invalid node index while parsing mesh");

	const auto& tn = model.nodes[nodeIndex];
	if (tn.mesh < 0 || tn.mesh >= static_cast<int>(model.meshes.size()))
		throw std::runtime_error("invalid mesh index on node");

	const auto& tnmesh = model.meshes[static_cast<size_t>(tn.mesh)];
	if (tnmesh.primitives.empty())
		throw std::runtime_error("mesh has no primitives");

	for (const auto& prim : tnmesh.primitives) {
		if (prim.mode != TINYGLTF_MODE_TRIANGLES)
			throw std::runtime_error("only TRIANGLES supported");

		const auto& attrs = prim.attributes;
		if (!attrs.contains("POSITION"))
			throw std::runtime_error("primitive has no POSITION attribute");

		const AccessorBytes pos = getAccessorBytes(model, attrs.at("POSITION"));
		if (pos.type != TINYGLTF_TYPE_VEC3 ||
			pos.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT ||
			pos.normalized) {
			throw std::runtime_error("POSITION must be a non-normalized FLOAT VEC3");
		}

		constexpr size_t maxU16VertexCount =
			static_cast<size_t>(std::numeric_limits<uint16_t>::max()) + 1u;
		if (pos.count > maxU16VertexCount)
			throw std::runtime_error("mesh exceeds 16-bit index capacity");

		AccessorBytes nor{};
		AccessorBytes uv{};
		const bool hasNormal = attrs.contains("NORMAL");
		const bool hasUv = attrs.contains("TEXCOORD_0");

		if (hasNormal) {
			nor = getAccessorBytes(model, attrs.at("NORMAL"));
			if (nor.type != TINYGLTF_TYPE_VEC3 ||
				nor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT ||
				nor.normalized || nor.count != pos.count) {
				throw std::runtime_error("NORMAL must be a matching FLOAT VEC3 accessor");
			}
		}

		if (hasUv) {
			uv = getAccessorBytes(model, attrs.at("TEXCOORD_0"));
			if (uv.type != TINYGLTF_TYPE_VEC2 ||
				uv.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT ||
				uv.normalized || uv.count != pos.count) {
				throw std::runtime_error("TEXCOORD_0 must be a matching FLOAT VEC2 accessor");
			}
		}

		const bool hasJoints = attrs.contains("JOINTS_0");
		const bool hasWeights = attrs.contains("WEIGHTS_0");
		if (hasJoints != hasWeights)
			throw std::runtime_error("JOINTS_0 and WEIGHTS_0 must be provided together");

		AccessorBytes jointAcc{};
		AccessorBytes weightAcc{};
		if (hasJoints) {
			jointAcc = getAccessorBytes(model, attrs.at("JOINTS_0"));
			if (jointAcc.type != TINYGLTF_TYPE_VEC4 ||
				(jointAcc.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE &&
				 jointAcc.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) ||
				jointAcc.normalized || jointAcc.count != pos.count) {
				throw std::runtime_error("JOINTS_0 must be a matching non-normalized UBYTE/USHORT VEC4 accessor");
			}

			weightAcc = getAccessorBytes(model, attrs.at("WEIGHTS_0"));
			const bool floatWeights =
				weightAcc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && !weightAcc.normalized;
			const bool normalizedIntegerWeights =
				(weightAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ||
				 weightAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) &&
				weightAcc.normalized;
			if (weightAcc.type != TINYGLTF_TYPE_VEC4 ||
				(!floatWeights && !normalizedIntegerWeights) ||
				weightAcc.count != pos.count) {
				throw std::runtime_error("WEIGHTS_0 must be a matching FLOAT or normalized UBYTE/USHORT VEC4 accessor");
			}
		}

		std::vector<uint16_t> idx;
		if (prim.indices >= 0) {
			const AccessorBytes indexAcc = getAccessorBytes(model, prim.indices);
			if (indexAcc.type != TINYGLTF_TYPE_SCALAR || indexAcc.normalized ||
				(indexAcc.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE &&
				 indexAcc.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT &&
				 indexAcc.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)) {
				throw std::runtime_error("indices must use a non-normalized unsigned SCALAR accessor");
			}
			if (indexAcc.count % 3 != 0)
				throw std::runtime_error("TRIANGLES index count must be divisible by three");

			idx.reserve(indexAcc.count);
			for (size_t i = 0; i < indexAcc.count; ++i) {
				const uint32_t value = readIndex(indexAcc, i);
				if (value > std::numeric_limits<uint16_t>::max())
					throw std::runtime_error("index exceeds the 16-bit mesh index limit");
				if (value >= pos.count)
					throw std::runtime_error("mesh index references a vertex out of range");
				idx.push_back(static_cast<uint16_t>(value));
			}
		} else {
			if (prim.indices != -1)
				throw std::runtime_error("invalid primitive indices value");
			if (pos.count % 3 != 0)
				throw std::runtime_error("non-indexed TRIANGLES vertex count must be divisible by three");

			idx.reserve(pos.count);
			for (size_t i = 0; i < pos.count; ++i)
				idx.push_back(static_cast<uint16_t>(i));
		}

		std::vector<Vertex> verts;
		verts.reserve(pos.count);
		for (size_t i = 0; i < pos.count; ++i) {
			const auto p = accessorFloatElement(pos, i);

			Vertex vert{};
			vert.x = p[0];
			vert.y = p[1];
			vert.z = p[2];

			if (hasNormal) {
				const auto n = accessorFloatElement(nor, i);
				vert.nx = n[0];
				vert.ny = n[1];
				vert.nz = n[2];
			}

			if (hasUv) {
				const auto u = accessorFloatElement(uv, i);
				vert.u = u[0];
				vert.v = u[1];
			}

			verts.push_back(vert);
		}

		if (hasJoints) {
			for (size_t i = 0; i < jointAcc.count; ++i)
				readJoints(verts[i], accessorElement(jointAcc, i), jointAcc.componentType);
			for (size_t i = 0; i < weightAcc.count; ++i)
				readWeights(verts[i], accessorElement(weightAcc, i), weightAcc.componentType);
		}

		Mesh mesh;
		if (prim.material >= 0) {
			if (prim.material >= static_cast<int>(model.materials.size()))
				throw std::runtime_error("invalid primitive material index");
			mesh.materialIndex = prim.material;
		} else if (prim.material != -1) {
			throw std::runtime_error("invalid primitive material value");
		}

		mesh.verts = std::move(verts);
		mesh.indices = std::move(idx);
		scalixModel.meshes.push_back(std::move(mesh));
	}
}


void GltfLoader::parse() {
	if (model.nodes.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
		throw std::runtime_error("too many nodes");

	const int nodesSize = static_cast<int>(model.nodes.size());
	scalixModel.nodes.resize(nodesSize);
	scalixModel.nodeHandles.resize(nodesSize);
	lnodes.resize(nodesSize);
	for (auto& lnode : lnodes)
		lnode.parent = NodeId::invalid();

	for (int i = 0; i < nodesSize; i++) {
		const auto& tn = model.nodes[static_cast<size_t>(i)];
		Node& node = scalixModel.nodes[static_cast<size_t>(i)];
		LoadingNode& lnode = lnodes[static_cast<size_t>(i)];

		if (tn.skin < -1 ||
			(tn.skin >= 0 && tn.skin >= static_cast<int>(model.skins.size()))) {
			throw std::runtime_error("invalid node skin index");
		}
		node.skinIndex = tn.skin;
		node.meshStartIndex = scalixModel.meshes.size();

		if (!tn.translation.empty()) {
			if (tn.translation.size() != 3)
				throw std::runtime_error("node translation must have three components");
			node.trs.pos.x = static_cast<float>(tn.translation[0]);
			node.trs.pos.y = static_cast<float>(tn.translation[1]);
			node.trs.pos.z = static_cast<float>(tn.translation[2]);
		}

		if (!tn.rotation.empty()) {
			if (tn.rotation.size() != 4)
				throw std::runtime_error("node rotation must have four components");
			node.trs.rot = {
				static_cast<float>(tn.rotation[0]),
				static_cast<float>(tn.rotation[1]),
				static_cast<float>(tn.rotation[2]),
				static_cast<float>(tn.rotation[3])
			};
		}

		if (!tn.scale.empty()) {
			if (tn.scale.size() != 3)
				throw std::runtime_error("node scale must have three components");
			node.trs.scale = {
				static_cast<float>(tn.scale[0]),
				static_cast<float>(tn.scale[1]),
				static_cast<float>(tn.scale[2])
			};
		}

		if (tn.mesh >= 0) {
			if (tn.mesh >= static_cast<int>(model.meshes.size()))
				throw std::runtime_error("invalid node mesh index");
			parseMesh(static_cast<NodeId>(i));
		} else if (tn.mesh != -1) {
			throw std::runtime_error("invalid node mesh value");
		}

		node.meshCount = scalixModel.meshes.size() - node.meshStartIndex;

		lnode.children.clear();
		lnode.children.reserve(tn.children.size());
		lnode.id = static_cast<NodeId>(i);
		node.name = strsv().entry(tn.name);

		for (const int child : tn.children) {
			if (child < 0 || child >= nodesSize)
				throw std::runtime_error("invalid child node index");
			if (child == i)
				throw std::runtime_error("node cannot be its own child");
			if (lnodes[static_cast<size_t>(child)].parent.isValid())
				throw std::runtime_error("node has multiple parents");

			lnodes[static_cast<size_t>(child)].parent = static_cast<NodeId>(i);
			lnode.children.push_back(static_cast<NodeId>(child));
		}
	}

	// Reject hierarchy cycles before node handles are resolved or transforms are evaluated.
	std::vector<uint8_t> visitState(nodesSize, 0);
	for (size_t start = 0; start < lnodes.size(); ++start) {
		std::vector<size_t> chain;
		NodeId current = static_cast<NodeId>(start);
		while (current.isValid()) {
			const size_t index = static_cast<uint32_t>(current);
			if (index >= lnodes.size())
				throw std::runtime_error("invalid parent node index");
			if (visitState[index] == 1)
				throw std::runtime_error("cycle detected in node hierarchy");
			if (visitState[index] == 2)
				break;

			visitState[index] = 1;
			chain.push_back(index);
			current = lnodes[index].parent;
		}
		for (const size_t index : chain)
			visitState[index] = 2;
	}

	for (const auto& model_skin : model.skins) {
		Skin skin;
		skin.joints.reserve(model_skin.joints.size());
		for (const int joint : model_skin.joints) {
			if (joint < 0 || joint >= nodesSize)
				throw std::runtime_error("skin joint references a node out of range");
			skin.joints.push_back(static_cast<NodeId>(joint));
		}

		if (model_skin.inverseBindMatrices >= 0) {
			const AccessorBytes invAcc = getAccessorBytes(model, model_skin.inverseBindMatrices);
			if (invAcc.type != TINYGLTF_TYPE_MAT4 ||
				invAcc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT ||
				invAcc.normalized || invAcc.count != skin.joints.size()) {
				throw std::runtime_error("inverseBindMatrices must be a matching FLOAT MAT4 accessor");
			}

			skin.invBind.resize(invAcc.count);
			for (size_t i = 0; i < invAcc.count; ++i) {
				const auto src = accessorFloatElement(invAcc, i);
				std::memcpy(skin.invBind[i].data(), src.data(), sizeof(float) * 16);
			}
		} else {
			if (model_skin.inverseBindMatrices != -1)
				throw std::runtime_error("invalid inverseBindMatrices accessor index");
			skin.invBind.resize(skin.joints.size());
			for (auto& matrix : skin.invBind)
				bx::mtxIdentity(matrix.data());
		}

		const int skinIndex = static_cast<int>(scalixModel.skins.size());
		for (size_t jointIndex = 0; jointIndex < skin.joints.size(); ++jointIndex) {
			const size_t nodeIndex = static_cast<uint32_t>(skin.joints[jointIndex]);
			auto& node = scalixModel.nodes[nodeIndex];
			node.skinIndex = skinIndex;
			node.jointIndex = static_cast<int>(jointIndex);
		}

		scalixModel.skins.push_back(std::move(skin));
	}
}


void GltfLoader::buildPalletCompress() {
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
					const size_t orig = vert.joints[i];
					if (orig >= remap.size())
						throw std::runtime_error("invalid joint index");
					if (vert.weights[i] <= 0.0001f) continue;

					// Compact the original skin joint index into this mesh palette.
					if (remap[orig] == -1) {
						remap[orig] = newIndex;
						remapInverse.push_back(static_cast<int>(orig));
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


GltfLoader::GltfLoader(const std::string& path, GltfType type): path(path), type(type) {}


void GltfLoader::load() {
	tinygltf::TinyGLTF loader;

	std::string err, warn;

	switch (type) {
		case GltfType::ascii: {
			if (!loader.LoadASCIIFromFile(&model, &err, &warn, path)) {
				throw std::runtime_error("gltf(ascii) load failed: " + err);
			}
		} break;

		case GltfType::binary: {
			if (!loader.LoadBinaryFromFile(&model, &err, &warn, path)) {
				throw std::runtime_error("gltf(binary) load failed: " + err);
			}
		} break;
	}

	if (model.meshes.empty())
		throw std::runtime_error("no mesh");

	parse();
	if (scalixModel.meshes.empty())
		throw std::runtime_error("no mesh attached to any node");

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

void GltfLoader::procHdl(Avatar* avatar) {
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

	std::vector<NodeId> origRoot;

	// parentとchildrenもhdl解決
	for (int i = 0; i < scalixModel.nodes.size(); i++) {
		Node& node = scalixModel.nodes[i];
		LoadingNode& lnode = lnodes[i];

		if (lnode.parent.isValid()) {
			node.parent = scalixModel.nodeHandles[lnode.parent];
		} else {
			node.parent = NodeHandle::invalid();
			origRoot.push_back(lnode.id);
		}

		node.children.resize(lnode.children.size());
		for (int j = 0; j < lnode.children.size(); j++) {
			NodeId& lchild = lnode.children[j];
			node.children[j] = scalixModel.nodeHandles[lchild];
		}
	}

	// 仮想rootを作成
	Node root;
	root.id = scalixModel.nodes.size();
	root.parent = NodeHandle::invalid();

	// 子のハンドルを登録
	for (NodeId rootId: origRoot) {
		const size_t index = static_cast<size_t>(rootId);
		root.children.push_back(scalixModel.nodeHandles[index]);
	}

	// nodesに追加してからハンドルを割り当て
	scalixModel.nodes.push_back(root);

	const NodeHandle rootHdl = nodeReg.create(avatar, root.id);
	scalixModel.root = rootHdl;
	scalixModel.nodeHandles.push_back(rootHdl);

	// 元のrootを仮想rootに接続
	for (NodeId rootId : origRoot) {
		const size_t index = static_cast<size_t>(rootId);
		scalixModel.nodes[index].parent = rootHdl;
	}

	handleSolved = true;

}
