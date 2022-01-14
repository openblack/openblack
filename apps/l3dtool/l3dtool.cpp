/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <L3DFile.h>

#include <cstdlib>
#include <stack>
#include <string>

#include <cxxopts.hpp>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

int PrintRawBytes(const void* data, std::size_t size)
{
	const uint32_t bytesPerLine = 0x10;
	std::printf("       00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
	std::printf("     ,------------------------------------------------\n");
	for (std::size_t y = 0; y < size / bytesPerLine; ++y)
	{
		auto base = static_cast<uint32_t>(y * bytesPerLine);
		printf("%04X |", base);
		for (uint32_t x = 0; x < bytesPerLine; ++x)
		{
			auto byte = reinterpret_cast<const uint8_t*>(data)[base + x];
			printf(" \033[%dm%02X\033[0m", byte == 0 ? 2 : 1, byte);
		}
		printf("\n");
	}
	auto remaining = size & (bytesPerLine - 1);
	if (remaining != 0)
	{
		auto base = static_cast<uint32_t>(size & ~(bytesPerLine - 1));
		printf("%04X |", base);
		for (std::size_t x = 0; x < remaining; ++x)
		{
			auto byte = reinterpret_cast<const uint8_t*>(data)[base + x];
			printf(" \033[%dm%02X\033[0m", byte == 0 ? 2 : 1, byte);
		}
		printf("\n");
	}

	return EXIT_SUCCESS;
}

int PrintHeader(openblack::l3d::L3DFile& l3d)
{
	auto& header = l3d.GetHeader();

	using L3DMeshFlags = openblack::l3d::L3DMeshFlags;
	auto flagToString = [](L3DMeshFlags flag) {
		std::string result;
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Unknown1))
		{
			result += "Unknown1|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Unknown2))
		{
			result += "Unknown2|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Unknown3))
		{
			result += "Unknown3|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Unknown4))
		{
			result += "Unknown4|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Unknown5))
		{
			result += "Unknown5|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Unknown6))
		{
			result += "Unknown6|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Unknown7))
		{
			result += "Unknown7|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Unknown8))
		{
			result += "Unknown8|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::HasBones))
		{
			result += "HasBones|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Unknown10))
		{
			result += "Unknown10|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Unknown11))
		{
			result += "Unknown11|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::HasDoorPosition))
		{
			result += "HasDoorPosition|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Packed))
		{
			result += "Packed|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::NoDraw))
		{
			result += "NoDraw|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Unknown15))
		{
			result += "Unknown15|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::ContainsLandscapeFeature))
		{
			result += "ContainsLandscapeFeature|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Unknown17))
		{
			result += "Unknown17|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Unknown18))
		{
			result += "Unknown18|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::ContainsUV2))
		{
			result += "ContainsUV2|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::ContainsNameData))
		{
			result += "ContainsNameData|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::ContainsExtraMetrics))
		{
			result += "ContainsExtraMetrics|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::ContainsEBone))
		{
			result += "ContainsEBone|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::ContainsTnLData))
		{
			result += "ContainsTnLData|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::ContainsNewEP))
		{
			result += "ContainsNewEP|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Unknown25))
		{
			result += "Unknown25|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Unknown26))
		{
			result += "Unknown26|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Unknown27))
		{
			result += "Unknown27|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Unknown28))
		{
			result += "Unknown28|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Unknown29))
		{
			result += "Unknown29|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Unknown30))
		{
			result += "Unknown30|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Unknown31))
		{
			result += "Unknown31|";
		}
		if (static_cast<uint32_t>(flag) & static_cast<uint32_t>(L3DMeshFlags::Unknown32))
		{
			result += "Unknown32|";
		}
		return result.size() > 1 ? result.substr(0, result.size() - 1) : result;
	};

	std::printf("file: %s\n", l3d.GetFilename().c_str());
	std::printf("magic: %s\n", std::string((char*)&header.magic, sizeof(header.magic)).c_str());
	std::printf("flags: %s\n", flagToString(header.flags).c_str());
	std::printf("size: %u\n", header.size);
	std::printf("submesh count: %u\n", header.submeshCount);
	std::printf("submesh start offset: 0x%08X\n", header.submeshOffsetsOffset);
	std::printf("bounding box:\n");
	std::printf("\tunknown 32 bits: 0x%08X\n", header.boundingBox.unknown);
	std::printf("\tcentre: {%f, %f, %f}\n", header.boundingBox.centre.x, header.boundingBox.centre.y,
	            header.boundingBox.centre.z);
	std::printf("\tsize: {%f, %f, %f}\n", header.boundingBox.size.x, header.boundingBox.size.y, header.boundingBox.size.z);
	std::printf("\tdiagonal length: %f\n", header.boundingBox.diagonalLength);
	std::printf("unknown offset: 0x%08X\n", header.anotherOffset);
	std::printf("skin count: %u\n", header.skinCount);
	std::printf("skin start offset: 0x%08X\n", header.skinOffsetsOffset);
	std::printf("extra data count: %u\n", header.extraDataCount);
	std::printf("extra data offset: 0x%08X\n", header.extraDataOffset);
	std::printf("footprint data offset: 0x%08X\n", header.footprintDataOffset);
	std::printf("\n");

	return EXIT_SUCCESS;
}

int PrintMeshHeaders(openblack::l3d::L3DFile& l3d)
{
	auto& meshHeaders = l3d.GetSubmeshHeaders();
	std::printf("file: %s\n", l3d.GetFilename().c_str());

	uint32_t i = 0;
	using Flags = openblack::l3d::L3DSubmeshHeader::Flags;
	auto flagToString = [](Flags flags) {
		std::string result;
		result += "LOD" + std::to_string(flags.lod);
		result += "|status=" + std::to_string(flags.status);
		if (flags.unknown1)
		{
			result += "|unknown1";
		}
		if (flags.isWindow)
		{
			result += "|isWindow";
		}
		if (flags.isPhysics)
		{
			result += "|isPhysics";
		}
		return result;
	};
	for (auto& header : meshHeaders)
	{
		std::printf("mesh #%u\n", ++i);
		std::printf("flags: %s\n", flagToString(header.flags).c_str());
		std::printf("primitive count: %u\n", header.numPrimitives);
		std::printf("primitives start offset: 0x%08X\n", header.primitivesOffset);
		std::printf("bone count: %u\n", header.numBones);
		std::printf("bones start offset: 0x%08X\n", header.bonesOffset);
		std::printf("\n");
	}

	return EXIT_SUCCESS;
}

int PrintSkins(openblack::l3d::L3DFile& l3d)
{
	auto& skins = l3d.GetSkins();
	std::printf("file: %s\n", l3d.GetFilename().c_str());

	for (auto& skin : skins)
	{
		std::printf("skin id: 0x%X\n", skin.id);
		std::printf("data:\n");
		constexpr uint16_t subsample = 8;
		constexpr uint16_t magnitude = (subsample * subsample) / 16;
		for (uint16_t y = 0; y < skin.height / subsample; ++y)
		{
			for (uint16_t x = 0; x < skin.width / subsample; ++x)
			{
				uint32_t red = 0;
				uint32_t green = 0;
				uint32_t blue = 0;
				for (uint16_t j = 0; j < subsample; ++j)
				{
					for (uint16_t i = 0; i < subsample; ++i)
					{
						auto& color = skin.texels[x * subsample + i + (y * subsample + j) * skin.width];
						red += color.R;
						green += color.G;
						blue += color.B;
					}
				}
				red /= magnitude;
				green /= magnitude;
				blue /= magnitude;
				std::printf("\x1b[48;2;%u;%u;%um  \x1b[0m", static_cast<uint8_t>(red), static_cast<uint8_t>(green),
				            static_cast<uint8_t>(blue));
			}
			std::printf("\n");
		}
	}

	return EXIT_SUCCESS;
}

int PrintExtraPoints(openblack::l3d::L3DFile& l3d)
{
	auto& points = l3d.GetExtraPoints();
	std::printf("file: %s\n", l3d.GetFilename().c_str());

	for (auto& point : points)
	{
		std::printf("(%.3f, %.3f, %.3f)\n", point.x, point.y, point.z);
	}

	return EXIT_SUCCESS;
}

int PrintPrimitiveHeaders(openblack::l3d::L3DFile& l3d)
{
	auto& primitiveHeaders = l3d.GetPrimitiveHeaders();
	std::printf("file: %s\n", l3d.GetFilename().c_str());

	uint32_t i = 0;
	for (auto& header : primitiveHeaders)
	{
		std::printf("primitive #%u\n", ++i);
		std::printf("material's type: 0x%08X\n", header.material.type);
		std::printf("material's alpha cut out threshold: 0x%02X (%f)\n", header.material.alphaCutoutThreshold,
		            header.material.alphaCutoutThreshold / 255.0f);
		std::printf("material's cull mode: 0x%02X\n", header.material.cullMode);
		std::printf("material's skinID: 0x%08X\n", header.material.skinID);
		std::printf("material's color: 0x%08X (%f, %f, %f, %f)\n", header.material.color.raw,
		            header.material.color.bgra.b / 255.0f, header.material.color.bgra.g / 255.0f,
		            header.material.color.bgra.r / 255.0f, header.material.color.bgra.a / 255.0f);
		std::printf("vertex count: %u\n", header.numVertices);
		std::printf("vertex start offset: 0x%08X\n", header.verticesOffset);
		std::printf("triangle count: %u\n", header.numTriangles);
		std::printf("triangle start offset: 0x%08X\n", header.trianglesOffset);
		std::printf("vertex group count: %u\n", header.numGroups);
		std::printf("vertex group start offset: 0x%08X\n", header.groupsOffset);
		std::printf("vertex blend count: %u\n", header.numVertexBlends);
		std::printf("vertex blend start offset: 0x%08X\n", header.vertexBlendsOffset);
		std::printf("\n");
	}

	return EXIT_SUCCESS;
}

int PrintBones(openblack::l3d::L3DFile& l3d)
{
	auto bones = l3d.GetBones();
	std::printf("file: %s\n", l3d.GetFilename().c_str());

	uint32_t i = 0;
	for (auto& bone : bones)
	{
		std::printf("bone #%u\n", i++);
		if (bone.parent == std::numeric_limits<uint32_t>::max())
		{
			std::printf("parent: None\n");
		}
		else
		{
			std::printf("parent: %X\n", bone.parent);
		}
		if (bone.firstChild == std::numeric_limits<uint32_t>::max())
		{
			std::printf("first child: None\n");
		}
		else
		{
			std::printf("first child: %X\n", bone.firstChild);
		}
		if (bone.rightSibling == std::numeric_limits<uint32_t>::max())
		{
			std::printf("right sibling: None\n");
		}
		else
		{
			std::printf("right sibling: %X\n", bone.rightSibling);
		}
		std::printf("rotation:\n");
		for (uint8_t j = 0; j < 3; ++j)
		{
			std::printf("     [%6.3f %6.3f %6.3f]\n", bone.orientation[j * 3], bone.orientation[j * 3 + 1],
			            bone.orientation[j * 3 + 2]);
		}
		std::printf("position: (%5.1f %5.1f %5.1f)\n", bone.position.x, bone.position.y, bone.position.z);
		std::printf("\n");
	}

	return EXIT_SUCCESS;
}

int PrintVertices(openblack::l3d::L3DFile& l3d)
{
	auto& vertices = l3d.GetVertices();
	std::printf("file: %s\n", l3d.GetFilename().c_str());
	std::printf("|     position     |   uv coord   |        normal        |\n");
	std::printf("|------------------|--------------|----------------------|\n");

	for (auto& vertex : vertices)
	{
		std::printf("|%5.1f %5.1f %5.1f |%6.3f %6.3f | %6.3f %6.3f %6.3f |\n", vertex.position.x, vertex.position.y,
		            vertex.position.z, vertex.texCoord.x, vertex.texCoord.y, vertex.normal.x, vertex.normal.y, vertex.normal.z);
	}

	return EXIT_SUCCESS;
}

int PrintIndices(openblack::l3d::L3DFile& l3d)
{
	constexpr uint8_t indexPerLine = 12;
	auto& indices = l3d.GetIndices();
	std::printf("file: %s\n", l3d.GetFilename().c_str());

	for (std::size_t i = 0; i < indices.size(); ++i)
	{
		std::printf("%s%4u", i == 0 ? "" : ((i % indexPerLine != 0) ? " " : "\n"), indices[i]);
	}
	std::printf("\n");

	return EXIT_SUCCESS;
}

int PrintLookUpTables(openblack::l3d::L3DFile& l3d)
{
	auto& lookUpTable = l3d.GetLookUpTableData();
	std::printf("file: %s\n", l3d.GetFilename().c_str());
	return PrintRawBytes(lookUpTable.data(), lookUpTable.size() * sizeof(lookUpTable[0]));
}

int PrintBlendValues(openblack::l3d::L3DFile& l3d)
{
	auto& blendValues = l3d.GetLookUpTableData();
	std::printf("file: %s\n", l3d.GetFilename().c_str());
	return PrintRawBytes(blendValues.data(), blendValues.size() * sizeof(blendValues[0]));
}

struct Arguments
{
	enum class Mode
	{
		Header,
		MeshHeader,
		Skin,
		ExtraPoint,
		PrimitiveHeader,
		Bones,
		Vertices,
		Indices,
		LookUpTables,
		VertexBlendValues,
		Write,
		Extract,
	};
	Mode mode;
	struct Read
	{
		std::vector<std::string> filenames;
	} read;
	struct Write
	{
		std::string outFilename;
		std::string gltfFile;
	} write;
	struct Extract
	{
		std::string inFilename;
		std::string gltfFile;
	} extract;
};

namespace details
{
template <typename dstT, typename srcT, typename = std::enable_if<std::is_same<dstT, srcT>::type>>
void copyBufferView(dstT* dst, const uint8_t* src, size_t count)
{
	for (size_t j = 0; j < count; ++j)
	{
		dst[j] = static_cast<dstT>(reinterpret_cast<const srcT*>(src)[j]);
	}
}

template <typename dstT, typename srcT>
void copyBufferView(dstT* dst, const uint8_t* src, size_t count)
{
	memcpy(dst, src, count * sizeof(dstT));
}
} // namespace details

template <typename desT>
void copyBufferView(desT* dst, const uint8_t* src, size_t count, uint32_t componentType)
{
	switch (componentType)
	{
	case TINYGLTF_COMPONENT_TYPE_BYTE:
		details::copyBufferView<desT, int8_t>(dst, src, count);
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		details::copyBufferView<desT, uint8_t>(dst, src, count);
		break;
	case TINYGLTF_COMPONENT_TYPE_SHORT:
		details::copyBufferView<desT, int16_t>(dst, src, count);
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		details::copyBufferView<desT, uint16_t>(dst, src, count);
		break;
	case TINYGLTF_COMPONENT_TYPE_INT:
		details::copyBufferView<desT, int32_t>(dst, src, count);
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		details::copyBufferView<desT, uint32_t>(dst, src, count);
		break;
	case TINYGLTF_COMPONENT_TYPE_FLOAT:
		details::copyBufferView<desT, float>(dst, src, count);
		break;
	case TINYGLTF_COMPONENT_TYPE_DOUBLE:
		details::copyBufferView<desT, double>(dst, src, count);
		break;
	default:
		throw std::runtime_error("Unsupported component type");
	}
}

bool NoopLoadImageDataFunction(tinygltf::Image*, const int, std::string*, std::string*, int, int, const unsigned char*, int,
                               void*)
{
	std::cerr << "Warn: GLTF Image Data function called but not implemented." << std::endl;
	return true;
}

int WriteFile(const Arguments::Write& args)
{
	openblack::l3d::L3DFile l3d {};

	if (args.gltfFile.empty())
	{
		return EXIT_FAILURE;
	}

	tinygltf::TinyGLTF loader;
	tinygltf::Model gltf;
	std::string err;
	std::string warn;

	loader.SetImageLoader(NoopLoadImageDataFunction, nullptr);
	bool ret = loader.LoadASCIIFromFile(&gltf, &err, &warn, args.gltfFile,
	                                    tinygltf::REQUIRE_VERSION | tinygltf::REQUIRE_ACCESSORS | tinygltf::REQUIRE_BUFFERS |
	                                        tinygltf::REQUIRE_BUFFER_VIEWS);

	if (!warn.empty())
	{
		std::cerr << "Warn: " << warn << std::endl;
	}

	if (!err.empty())
	{
		std::cerr << "Err: " << err << std::endl;
	}

	if (!ret)
	{
		throw std::runtime_error("Failed to parse glTF.");
	}

	if (gltf.meshes.empty())
	{
		throw std::runtime_error("There are no meshes in glTF.");
	}

	for (auto& gltfNode : gltf.nodes)
	{
		if (gltfNode.mesh < 0)
		{
			continue;
		}

		auto& gltfMesh = gltf.meshes[gltfNode.mesh];
		using L3DSubmeshHeader = openblack::l3d::L3DSubmeshHeader;
		L3DSubmeshHeader submesh;
		submesh.flags.lod = 0;
		submesh.flags.status = 0;
		submesh.flags.unknown1 = 0b0101;
		submesh.flags.isWindow = false;
		submesh.flags.isPhysics = false;
		submesh.flags.unknown2 = 0;
		submesh.numPrimitives = 0;
		submesh.primitivesOffset = static_cast<uint32_t>(l3d.GetPrimitiveHeaders().size());
		submesh.numBones = 0;
		submesh.bonesOffset = static_cast<uint32_t>(l3d.GetBones().size());

		// Bones
		if (gltfNode.skin >= 0)
		{
			auto& skin = gltf.skins[gltfNode.skin];
			auto skelton = skin.skeleton;
			if (skelton < 0)
			{
				if (!skin.joints.empty())
				{
					skelton = skin.joints.front();
				}
			}
			if (skelton >= 0)
			{
				std::vector<openblack::l3d::L3DBone> bones;
				std::function<void(const std::vector<int>&, uint32_t parent)> buildJoints;
				buildJoints = [&gltf, &bones, &buildJoints](const std::vector<int>& children, uint32_t parentId) {
					openblack::l3d::L3DBone* leftSibbling = nullptr;
					for (uint32_t i = 0; i < children.size(); ++i)
					{
						auto& gltfJoint = gltf.nodes[children[i]];

						auto id = static_cast<uint32_t>(bones.size());
						bones.emplace_back();
						auto& bone = bones.back();
						bone.parent = parentId;
						bone.firstChild = std::numeric_limits<uint32_t>::max();
						bone.rightSibling = std::numeric_limits<uint32_t>::max();
						// Convert Quaternion to matrix
						bone.orientation[0] = static_cast<float>(1.0f - 2.0f * gltfJoint.rotation[1] * gltfJoint.rotation[1] -
						                                         2.0f * gltfJoint.rotation[2] * gltfJoint.rotation[2]);
						bone.orientation[1] = static_cast<float>(2.0f * gltfJoint.rotation[0] * gltfJoint.rotation[1] -
						                                         2.0f * gltfJoint.rotation[2] * gltfJoint.rotation[3]);
						bone.orientation[2] = static_cast<float>(2.0f * gltfJoint.rotation[0] * gltfJoint.rotation[2] +
						                                         2.0f * gltfJoint.rotation[1] * gltfJoint.rotation[3]);
						bone.orientation[3] = static_cast<float>(2.0f * gltfJoint.rotation[0] * gltfJoint.rotation[1] +
						                                         2.0f * gltfJoint.rotation[2] * gltfJoint.rotation[3]);
						bone.orientation[4] = static_cast<float>(1.0f - 2.0f * gltfJoint.rotation[0] * gltfJoint.rotation[0] -
						                                         2.0f * gltfJoint.rotation[2] * gltfJoint.rotation[2]);
						bone.orientation[5] = static_cast<float>(2.0f * gltfJoint.rotation[1] * gltfJoint.rotation[2] -
						                                         2.0f * gltfJoint.rotation[0] * gltfJoint.rotation[3]);
						bone.orientation[6] = static_cast<float>(2.0f * gltfJoint.rotation[0] * gltfJoint.rotation[2] -
						                                         2.0f * gltfJoint.rotation[1] * gltfJoint.rotation[3]);
						bone.orientation[7] = static_cast<float>(2.0f * gltfJoint.rotation[1] * gltfJoint.rotation[2] +
						                                         2.0f * gltfJoint.rotation[0] * gltfJoint.rotation[3]);
						bone.orientation[8] = static_cast<float>(1.0f - 2.0f * gltfJoint.rotation[0] * gltfJoint.rotation[0] -
						                                         2.0f * gltfJoint.rotation[1] * gltfJoint.rotation[1]);
						bone.position.x = static_cast<float>(gltfJoint.translation[0]);
						bone.position.y = static_cast<float>(gltfJoint.translation[1]);
						bone.position.z = static_cast<float>(gltfJoint.translation[2]);
						buildJoints(gltfJoint.children, id);
						if (leftSibbling)
						{
							leftSibbling->rightSibling = id;
						}
						leftSibbling = &bone;
					}
				};

				buildJoints({skelton}, std::numeric_limits<uint32_t>::max());

				l3d.AddBones(bones);
				submesh.numBones += static_cast<uint32_t>(bones.size());
			}
		}

		// Primitives
		std::vector<openblack::l3d::L3DPrimitiveHeader> primitives;
		primitives.resize(gltfMesh.primitives.size());
		for (size_t i = 0; i < gltfMesh.primitives.size(); ++i)
		{
			auto& gltfPrimitive = gltfMesh.primitives[i];
			auto& primitive = primitives[i];
			primitive.material.type = 0;
			primitive.material.alphaCutoutThreshold = 0;
			primitive.material.skinID = std::numeric_limits<uint32_t>::max(); // TODO: mesh->mMaterialIndex;
			primitive.material.color.raw = 0x00000000;
			primitive.numVertices = 0;
			primitive.verticesOffset = static_cast<uint32_t>(l3d.GetVertices().size()); // FIXME: This is wrong
			primitive.numTriangles = 0;
			primitive.trianglesOffset = static_cast<uint32_t>(l3d.GetIndices().size() / 3);  // FIXME: This is wrong
			primitive.numGroups = 0;                                                         // TODO: Figure this out
			primitive.groupsOffset = static_cast<uint32_t>(l3d.GetLookUpTableData().size()); // FIXME: This is wrong
			primitive.numVertexBlends = 0;                                                   // TODO: Figure this out
			primitive.vertexBlendsOffset = static_cast<uint32_t>(l3d.GetBlends().size());    // FIXME: This is wrong

			struct attribute_t
			{
				std::string name;
				std::vector<float> values;
				uint8_t type;
			};
			attribute_t attributes[3] = {
			    {"POSITION", {}, 0},
			    {"TEXCOORD_0", {}, 0},
			    {"NORMAL", {}, 0},
			};
			uint32_t count = 0;
			for (auto& attribute : attributes)
			{
				auto iter = gltfPrimitive.attributes.find(attribute.name);
				if (iter != gltfPrimitive.attributes.end())
				{
					auto& accessor = gltf.accessors[iter->second];
					auto& view = gltf.bufferViews[accessor.bufferView];
					auto& buffer = gltf.buffers[view.buffer];
					auto offset = view.byteOffset + accessor.byteOffset;
					attribute.values.resize(accessor.count * accessor.type);
					copyBufferView(attribute.values.data(), buffer.data.data() + offset, attribute.values.size(),
					               accessor.componentType);
					attribute.type = static_cast<uint8_t>(accessor.type);
					if (count != 0 && count != accessor.count)
					{
						throw std::runtime_error("Attributes do not match in length.");
					}
					count = static_cast<uint32_t>(accessor.count);
				}
			}

			if (count == 0)
			{
				throw std::runtime_error("No vertex attributes found.");
			}

			std::vector<openblack::l3d::L3DVertex> vertices;
			vertices.resize(count);
			for (uint32_t j = 0; j < count; ++j)
			{
				auto& vertex = vertices[j];
				if (attributes[0].type)
				{
					vertex.position.x = attributes[0].values[j * attributes[0].type + 0];
					vertex.position.y = attributes[0].values[j * attributes[0].type + 1];
					vertex.position.z = attributes[0].values[j * attributes[0].type + 2];
				}
				if (attributes[1].type)
				{
					vertex.texCoord.x = attributes[1].values[j * attributes[1].type + 0];
					vertex.texCoord.y = attributes[1].values[j * attributes[1].type + 1];
				}
				if (attributes[2].type)
				{
					vertex.normal.x = attributes[2].values[j * attributes[2].type + 0];
					vertex.normal.y = attributes[2].values[j * attributes[2].type + 1];
					vertex.normal.z = attributes[2].values[j * attributes[2].type + 2];
				}
			}
			l3d.AddVertices(vertices);
			primitive.numVertices += static_cast<uint32_t>(vertices.size());

			{
				auto& accessor = gltf.accessors[gltfPrimitive.indices];
				auto& view = gltf.bufferViews[accessor.bufferView];
				auto& buffer = gltf.buffers[view.buffer];
				auto offset = view.byteOffset + accessor.byteOffset;
				std::vector<uint16_t> indices;
				indices.resize(accessor.count);
				copyBufferView(indices.data(), buffer.data.data() + offset, indices.size(), accessor.componentType);
				l3d.AddIndices(indices);
				primitive.numTriangles += static_cast<uint32_t>(indices.size() / 3);
			}
		}
		l3d.AddPrimitives(primitives);
		submesh.numPrimitives += static_cast<uint32_t>(primitives.size());

		l3d.AddSubmesh(submesh);
	}

	l3d.Write(args.outFilename);

	return EXIT_SUCCESS;
}

int ExtractFile(const Arguments::Extract& args)
{
	openblack::l3d::L3DFile l3d {};

	if (args.gltfFile.empty() || args.inFilename.empty())
	{
		return EXIT_FAILURE;
	}

	// Open file
	l3d.Open(args.inFilename);

	tinygltf::Model gltf;

	gltf.asset.version = "2.0";
	gltf.asset.generator = "l3dtool extract";

	// Data buffer
	tinygltf::Buffer vertexBuffer; // 0
	size_t sizeOfVertices = l3d.GetVertices().size() * sizeof(l3d.GetVertices()[0]);
	vertexBuffer.data.resize(sizeOfVertices);
	memcpy(vertexBuffer.data.data(), l3d.GetVertices().data(), sizeOfVertices);
	gltf.buffers.push_back(vertexBuffer);

	tinygltf::Buffer indexBuffer; // 1
	size_t sizeOfIndices = l3d.GetIndices().size() * sizeof(l3d.GetIndices()[0]);
	indexBuffer.data.resize(sizeOfIndices);
	memcpy(indexBuffer.data.data(), l3d.GetIndices().data(), sizeOfIndices);
	gltf.buffers.push_back(indexBuffer);

	// TODO: weights, skins

	// Buffer views
	tinygltf::BufferView vertexView; // 0
	vertexView.name = "vertex position view";
	vertexView.buffer = 0;
	vertexView.byteOffset = 0;
	vertexView.byteLength = sizeOfVertices;
	vertexView.byteStride = sizeof(openblack::l3d::L3DVertex);
	vertexView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
	gltf.bufferViews.push_back(vertexView);
	tinygltf::BufferView indexView; // 1
	indexView.name = "index view";
	indexView.buffer = 1;
	indexView.byteOffset = 0;
	indexView.byteLength = sizeOfIndices;
	indexView.byteStride = 0;
	indexView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
	gltf.bufferViews.push_back(indexView);

	auto rootNodeIndex = static_cast<uint32_t>(gltf.nodes.size());
	gltf.nodes.emplace_back().name = "root node"; // 0

	// Scene
	tinygltf::Scene scene;
	scene.name = "l3dtool exported scene from " + args.inFilename;
	scene.nodes.push_back(0);
	gltf.scenes.push_back(scene);
	gltf.defaultScene = 0;

	// Meshes
	uint32_t vertexOffset = 0;
	uint32_t indexOffset = 0;
	gltf.meshes.resize(l3d.GetSubmeshHeaders().size());
	for (uint32_t i = 0; i < l3d.GetSubmeshHeaders().size(); ++i)
	{
		auto& gltfMesh = gltf.meshes[i];
		gltfMesh.name = "submesh #" + std::to_string(i);
		gltfMesh.primitives.resize(l3d.GetPrimitiveSpan(i).size());
		for (uint32_t j = 0; j < l3d.GetPrimitiveSpan(i).size(); ++j)
		{
			auto& gltfPrimitive = gltfMesh.primitives[j];
			auto& l3dPrimitive = l3d.GetPrimitiveSpan(i)[j];

			// Accessors
			tinygltf::Accessor vertexPositionAccessor;
			vertexPositionAccessor.bufferView = 0;
			vertexPositionAccessor.name =
			    "vertex position accessor for primitive #" + std::to_string(j) + " of " + gltfMesh.name;
			vertexPositionAccessor.byteOffset =
			    vertexOffset * sizeof(openblack::l3d::L3DVertex) + offsetof(openblack::l3d::L3DVertex, position);
			vertexPositionAccessor.normalized = false;
			vertexPositionAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			vertexPositionAccessor.count = l3dPrimitive.numVertices;
			vertexPositionAccessor.type = TINYGLTF_TYPE_VEC3;
			gltfPrimitive.attributes["POSITION"] = static_cast<int>(gltf.accessors.size());
			gltf.accessors.push_back(vertexPositionAccessor);

			tinygltf::Accessor vertexTextureCoordinateAccessor;
			vertexTextureCoordinateAccessor.bufferView = 0;
			vertexTextureCoordinateAccessor.name =
			    "vertex texture coordinate accessor for primitive #" + std::to_string(j) + " of " + gltfMesh.name;
			vertexTextureCoordinateAccessor.byteOffset =
			    vertexOffset * sizeof(openblack::l3d::L3DVertex) + offsetof(openblack::l3d::L3DVertex, texCoord);
			vertexTextureCoordinateAccessor.normalized = false;
			vertexTextureCoordinateAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			vertexTextureCoordinateAccessor.count = l3dPrimitive.numVertices;
			vertexTextureCoordinateAccessor.type = TINYGLTF_TYPE_VEC2;
			gltfPrimitive.attributes["TEXCOORD_0"] = static_cast<int>(gltf.accessors.size());
			gltf.accessors.push_back(vertexTextureCoordinateAccessor);

			tinygltf::Accessor vertexNormalAccessor;
			vertexNormalAccessor.bufferView = 0;
			vertexNormalAccessor.name = "vertex normal accessor for primitive #" + std::to_string(j) + " of " + gltfMesh.name;
			vertexNormalAccessor.byteOffset =
			    vertexOffset * sizeof(openblack::l3d::L3DVertex) + offsetof(openblack::l3d::L3DVertex, normal);
			vertexNormalAccessor.normalized = false;
			vertexNormalAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			vertexNormalAccessor.count = l3dPrimitive.numVertices;
			vertexNormalAccessor.type = TINYGLTF_TYPE_VEC3;
			gltfPrimitive.attributes["NORMAL"] = static_cast<int>(gltf.accessors.size());
			gltf.accessors.push_back(vertexNormalAccessor);

			tinygltf::Accessor indexAccessor;
			indexAccessor.bufferView = 1;
			indexAccessor.name = "index accessor for primitive #" + std::to_string(j) + " of " + gltfMesh.name;
			indexAccessor.byteOffset = indexOffset * sizeof(uint16_t);
			indexAccessor.normalized = false;
			indexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
			indexAccessor.count = l3dPrimitive.numTriangles * 3;
			indexAccessor.type = TINYGLTF_TYPE_SCALAR;
			gltfPrimitive.indices = static_cast<int>(gltf.accessors.size());
			gltf.accessors.push_back(indexAccessor);

			// TODO gltfPrimitive.material;
			gltfPrimitive.mode = TINYGLTF_MODE_TRIANGLES;
			// TODO gltfPrimitive.targets

			vertexOffset += l3dPrimitive.numVertices;
			indexOffset += l3dPrimitive.numTriangles * 3;
		}
		// TODO gltfMesh.weights
		gltf.nodes[rootNodeIndex].children.push_back(static_cast<int>(gltf.nodes.size()));
		tinygltf::Node node;
		node.name = gltfMesh.name + " node";
		node.mesh = static_cast<int>(i);
		gltf.nodes.push_back(node);
	}

	auto& bones = l3d.GetBones();
	std::vector<uint32_t> roots;
	for (uint32_t i = 0; i < bones.size(); ++i)
	{
		if (bones[i].parent == std::numeric_limits<uint32_t>::max())
		{
			roots.push_back(i);
		}
	}

	for (uint32_t i : roots)
	{
		auto bone_to_node = [](tinygltf::Node& node, const openblack::l3d::L3DBone& bone) {
			node.translation.clear();
			node.translation.push_back(bone.position.x);
			node.translation.push_back(bone.position.y);
			node.translation.push_back(bone.position.z);

			node.rotation.clear();
			auto& w = node.rotation.emplace_back(1);
			auto& x = node.rotation.emplace_back(0);
			auto& y = node.rotation.emplace_back(0);
			auto& z = node.rotation.emplace_back(0);

			// From glm quat_cast
			float fourXSquaredMinus1 = bone.orientation[0] - bone.orientation[4] - bone.orientation[8];
			float fourYSquaredMinus1 = bone.orientation[4] - bone.orientation[0] - bone.orientation[8];
			float fourZSquaredMinus1 = bone.orientation[8] - bone.orientation[0] - bone.orientation[4];
			float fourWSquaredMinus1 = bone.orientation[0] + bone.orientation[4] + bone.orientation[8];

			int biggestIndex = 0;
			float fourBiggestSquaredMinus1 = fourWSquaredMinus1;
			if (fourXSquaredMinus1 > fourBiggestSquaredMinus1)
			{
				fourBiggestSquaredMinus1 = fourXSquaredMinus1;
				biggestIndex = 1;
			}
			if (fourYSquaredMinus1 > fourBiggestSquaredMinus1)
			{
				fourBiggestSquaredMinus1 = fourYSquaredMinus1;
				biggestIndex = 2;
			}
			if (fourZSquaredMinus1 > fourBiggestSquaredMinus1)
			{
				fourBiggestSquaredMinus1 = fourZSquaredMinus1;
				biggestIndex = 3;
			}

			float biggestVal = -std::sqrt(fourBiggestSquaredMinus1 + 1.0f) * 0.5f;
			float mult = 0.25f / biggestVal;

			switch (biggestIndex)
			{
			case 0:
				w = biggestVal;
				x = (bone.orientation[5] - bone.orientation[7]) * mult;
				y = (bone.orientation[6] - bone.orientation[2]) * mult;
				z = (bone.orientation[1] - bone.orientation[3]) * mult;
				break;
			case 1:
				w = (bone.orientation[5] - bone.orientation[7]) * mult;
				x = biggestVal;
				y = (bone.orientation[1] - bone.orientation[3]) * mult;
				z = (bone.orientation[6] - bone.orientation[2]) * mult;
				break;
			case 2:
				w = (bone.orientation[6] - bone.orientation[2]) * mult;
				x = (bone.orientation[1] - bone.orientation[3]) * mult;
				y = biggestVal;
				z = (bone.orientation[5] - bone.orientation[7]) * mult;
				break;
			case 3:
				w = (bone.orientation[1] - bone.orientation[3]) * mult;
				x = (bone.orientation[6] - bone.orientation[2]) * mult;
				y = (bone.orientation[5] - bone.orientation[7]) * mult;
				z = biggestVal;
				break;
			}
		};

		// l3d index of current node + gltf index of parent node
		std::stack<std::pair<uint32_t, uint32_t>> traversal_stack;
		traversal_stack.emplace(i, rootNodeIndex);
		tinygltf::Node node;
		uint32_t index, parent_index;
		while (!traversal_stack.empty())
		{
			std::tie(index, parent_index) = traversal_stack.top();
			traversal_stack.pop();
			if (parent_index == rootNodeIndex)
			{
				node.name = "Bones root node #" + std::to_string(i);
			}
			else
			{
				node.name = "Bone Child #" + std::to_string(index) + " of #" + std::to_string(i);
			}

			gltf.nodes[parent_index].children.push_back(static_cast<int>(gltf.nodes.size()));
			auto& l3d_node = bones[index];
			bone_to_node(node, l3d_node);
			gltf.nodes.emplace_back(node);

			if (l3d_node.rightSibling != std::numeric_limits<uint32_t>::max())
			{
				traversal_stack.emplace(l3d_node.rightSibling, parent_index);
			}

			if (l3d_node.firstChild != std::numeric_limits<uint32_t>::max())
			{
				traversal_stack.emplace(l3d_node.firstChild, static_cast<uint32_t>(gltf.nodes.size() - 1));
			}
		}
	}
	// TODO: Associate mesh and joints to node

	tinygltf::TinyGLTF exporter;
	exporter.WriteGltfSceneToFile(&gltf, args.gltfFile, true, true, true, false);

	return EXIT_SUCCESS;
}

bool parseOptions(int argc, char** argv, Arguments& args, int& return_code)
{
	cxxopts::Options options("l3dtool", "Inspect and extract files from LionHead L3D files.");

	// clang-format off
	options.add_options()
		("h,help", "Display this help message.")
		("subcommand", "Subcommand.", cxxopts::value<std::string>())
	;
	options.positional_help("[read|write|extract] [OPTION...]");
	options.add_options("read")
		("H,header", "Print Header Contents.", cxxopts::value<std::vector<std::string>>())
		("m,mesh-header", "Print Mesh Headers.", cxxopts::value<std::vector<std::string>>())
		("s,skins", "Print Skins.", cxxopts::value<std::vector<std::string>>())
		("p,extra-points", "Print Extra Points.", cxxopts::value<std::vector<std::string>>())
		("P,primitive-header", "Print Primitive Headers.", cxxopts::value<std::vector<std::string>>())
		("b,bones", "Print Bones.", cxxopts::value<std::vector<std::string>>())
		("V,vertices", "Print Vertices.", cxxopts::value<std::vector<std::string>>())
		("I,indices", "Print Indices.", cxxopts::value<std::vector<std::string>>())
		("L,look-up-tables", "Print Look Up Table Data.", cxxopts::value<std::vector<std::string>>())
		("B,vertex-blend-values", "Print Vertex Blend Values.", cxxopts::value<std::vector<std::string>>())
	;
	options.add_options("write/extract from and to glTF format")
		("o,output", "Output file (required).", cxxopts::value<std::string>())
		("i,input-mesh", "Input file (required).", cxxopts::value<std::string>())
	;
	// clang-format on

	options.parse_positional({"subcommand"});

	try
	{
		auto result = options.parse(argc, argv);
		if (result["help"].as<bool>())
		{
			std::cout << options.help() << std::endl;
			return_code = EXIT_SUCCESS;
			return false;
		}
		if (result["subcommand"].count() == 0)
		{
			std::cerr << options.help() << std::endl;
			return_code = EXIT_FAILURE;
			return false;
		}
		if (result["subcommand"].as<std::string>() == "read")
		{
			if (result["header"].count() > 0)
			{
				args.mode = Arguments::Mode::Header;
				args.read.filenames = result["header"].as<std::vector<std::string>>();
				return true;
			}
			if (result["mesh-header"].count() > 0)
			{
				args.mode = Arguments::Mode::MeshHeader;
				args.read.filenames = result["mesh-header"].as<std::vector<std::string>>();
				return true;
			}
			if (result["skins"].count() > 0)
			{
				args.mode = Arguments::Mode::Skin;
				args.read.filenames = result["skins"].as<std::vector<std::string>>();
				return true;
			}
			if (result["extra-points"].count() > 0)
			{
				args.mode = Arguments::Mode::ExtraPoint;
				args.read.filenames = result["extra-points"].as<std::vector<std::string>>();
				return true;
			}
			if (result["primitive-header"].count() > 0)
			{
				args.mode = Arguments::Mode::PrimitiveHeader;
				args.read.filenames = result["primitive-header"].as<std::vector<std::string>>();
				return true;
			}
			if (result["bones"].count() > 0)
			{
				args.mode = Arguments::Mode::Bones;
				args.read.filenames = result["bones"].as<std::vector<std::string>>();
				return true;
			}
			if (result["vertices"].count() > 0)
			{
				args.mode = Arguments::Mode::Vertices;
				args.read.filenames = result["vertices"].as<std::vector<std::string>>();
				return true;
			}
			if (result["indices"].count() > 0)
			{
				args.mode = Arguments::Mode::Indices;
				args.read.filenames = result["indices"].as<std::vector<std::string>>();
				return true;
			}
			if (result["look-up-tables"].count() > 0)
			{
				args.mode = Arguments::Mode::LookUpTables;
				args.read.filenames = result["look-up-tables"].as<std::vector<std::string>>();
				return true;
			}
			if (result["vertex-blend-values"].count() > 0)
			{
				args.mode = Arguments::Mode::VertexBlendValues;
				args.read.filenames = result["vertex-blend-values"].as<std::vector<std::string>>();
				return true;
			}
		}
		else if (result["subcommand"].as<std::string>() == "write")
		{
			args.write.outFilename = "";
			if (result["output"].count() > 0)
			{
				args.mode = Arguments::Mode::Write;
				args.write.outFilename = result["output"].as<std::string>();
				if (result["input-mesh"].count() > 0)
				{
					args.write.gltfFile = result["input-mesh"].as<std::string>();
				}
				return true;
			}
		}
		else if (result["subcommand"].as<std::string>() == "extract")
		{
			args.write.outFilename = "";
			if (result["output"].count() > 0)
			{
				args.mode = Arguments::Mode::Extract;
				args.extract.inFilename = result["input-mesh"].as<std::string>();
				if (result["input-mesh"].count() > 0)
				{
					args.extract.gltfFile = result["output"].as<std::string>();
				}
				return true;
			}
		}
	}
	catch (cxxopts::OptionParseException& err)
	{
		std::cerr << err.what() << std::endl;
	}

	std::cerr << options.help() << std::endl;
	return_code = EXIT_FAILURE;
	return false;
}

int main(int argc, char* argv[])
{
	Arguments args;
	int return_code = EXIT_SUCCESS;
	if (!parseOptions(argc, argv, args, return_code))
	{
		return return_code;
	}

	if (args.mode == Arguments::Mode::Write)
	{
		return WriteFile(args.write);
	}

	if (args.mode == Arguments::Mode::Extract)
	{
		return ExtractFile(args.extract);
	}

	for (auto& filename : args.read.filenames)
	{
		openblack::l3d::L3DFile l3d;
		try
		{
			// Open file
			l3d.Open(filename);

			switch (args.mode)
			{
			case Arguments::Mode::Header:
				return_code |= PrintHeader(l3d);
				break;
			case Arguments::Mode::MeshHeader:
				return_code |= PrintMeshHeaders(l3d);
				break;
			case Arguments::Mode::Skin:
				return_code |= PrintSkins(l3d);
				break;
			case Arguments::Mode::ExtraPoint:
				return_code |= PrintExtraPoints(l3d);
				break;
			case Arguments::Mode::PrimitiveHeader:
				return_code |= PrintPrimitiveHeaders(l3d);
				break;
			case Arguments::Mode::Bones:
				return_code |= PrintBones(l3d);
				break;
			case Arguments::Mode::Vertices:
				return_code |= PrintVertices(l3d);
				break;
			case Arguments::Mode::Indices:
				return_code |= PrintIndices(l3d);
				break;
			case Arguments::Mode::LookUpTables:
				return_code |= PrintLookUpTables(l3d);
				break;
			case Arguments::Mode::VertexBlendValues:
				return_code |= PrintBlendValues(l3d);
				break;
			default:
				return_code = EXIT_FAILURE;
				break;
			}
		}
		catch (std::runtime_error& err)
		{
			std::cerr << err.what() << std::endl;
			return_code |= EXIT_FAILURE;
		}
	}

	return return_code;
}
