/*******************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************
 *
 * The layout of a L3D File is as follows:
 *
 * - 76 byte header, containing 19 ints
 *         magic number, containing 4 chars "L3D0"
 *         flags
 *         submesh count - count of the meshes in submesh block (see below)
 *         submesh start offset - offset of the submesh block (see below)
 *         the bounding box - initialized to zeros
 *         unknown offset
 *         skin count - count of the skins in skin block (see below)
 *         skin start offset - offset of the skin block (see below)
 *         extra data count - count of the points in point block (see below)
 *         extra data offset - offset of the point block (see below)
 *         footprint data offset - offset of the footprint block (see below)
 *
 * There are a few optional blocks. Their presence can be check for using the flags field
 * The footprint block is present when ContainsLandscapeFeature is set
 * The UV2 block is present when ContainsUV2 is set
 * The name block is present when ContainsNameData is set
 *
 * Add the footprint block offset and size to get to the UV2 block offset
 * Add the UV2 offset and size to get the name block offset
 *
 * ------------------------ start of submesh offset block ----------------------
 *
 * - 4 bytes * submesh count, each record contains:
 *         offset into submesh buffer (see below)
 *
 * ------------------------ start of skin offset block -------------------------
 *
 * - 4 bytes * skin count, each record contains:
 *         offset into skin buffer (see below)
 *
 * ------------------------ start of extra block -------------------------------
 *
 *  Depending on the flags set, these include data blocks that can be:
 * - With HasDoorPosition: 12 bytes, containing point data as floats for x, y, z
 *
 * ------------------------ start of footprint block ----------------------------
 *
 *  TODO(#484): Investigate optional footprint block
 *
 * ------------------------ start of uv2 block ---------------------------------
 *
 *  TODO(#483): Investigate optional UV2 block
 *
 * ------------------------ start of name block --------------------------------
 * - 4 bytes + total bytes used for the name.
 *         name size - total length of name (see below). This can be 0
 *         name - The name itself. Null terminated
 *
 * ------------------------ start of submesh block -----------------------------
 *
 * - 20 bytes header * total submeshes, each record containing 5 ints:
 *         flags
 *         primitive count - count of primitives in primitive block
 *                           (see below)
 *         primitive offsets offset - offset of the primitive offsets in
 *                                    primitive offsets block (see below)
 *         bone count - count of primitives in primitive block (see below)
 *         bone offset - offset of the primitives in primitive block
 *                       (see below)
 *
 * ------------------------ start of skin block --------------------------------
 *
 * - 131076 bytes * total skins, each record containing:
 *         id - a hashed key
 *         color - 16 bit texels encoded in R4G4B4A4 at 256 * 256 resolution
 *
 * ------------------------ start of primitive offset block --------------------
 *
 * - 4 bytes * total primitives, each record contains:
 *         offset into primitive buffer (see below)
 *
 * ------------------------ start of primitive block ---------------------------
 *
 * - 48 bytes header * total primitives, each record containing 12 ints
 *         material type - 32 bit rendering method to be used
 *         alpha cutout threshold - 8 bit alpha value threshold to dismiss
 *         cull mode - 8 bit culling method
 *         unused 16 bits
 *         skin id: id of the skin to use (see above)
 *         color - 32 bit color in BGRA8 format
 *         vertex count: count of the vertices in vertex block (see below)
 *         vertex start offset: offset in vertex block (see below)
 *         triangle count: count of the triangles in triangle block
 *                         (see below)
 *         triangle start offset: offset in triangle block (see below)
 *         vertex groups count: count of the bone-vertex association groups
 *                              (see below)
 *         vertex groups start offset: offset in look-up table block (see below)
 *         blend count: count of the vertex blend values in vertex blend
 *                      block (see below)
 *         blend offset: count of the vertex blend values in vertex blend
 *                       block (see below)
 *
 * ------------------------ start of bone block --------------------------------
 *
 * - 27 bytes * total bones, each record containing
 *         parent - offset of parent node, -1 if there are none
 *         child - offset of left-most child node, -1 if there are none
 *         sibling - offset of right sibling node, -1 if there are none
 *         orientation - 3x3 float rotation matrix
 *         position - 3 float position vector
 *
 * ------------------------ start of vertex block ------------------------------
 *
 * - 32 bytes * total vertices, each record containing:
 *         position: 3 floats representing position in model space
 *         texture coordinates: 2 floats representing texture coordinates
 *         normals: 3 floats representing surface normals at a point
 *
 * ------------------------ start of triangle block ----------------------------
 *
 * - 6 bytes * total triangles, each record containing three vertex indices
 *
 * ------------------------ start of vertex group block ------------------------
 *
 * - 4 bytes * total vertex groups, each record containing two 16 bit ints
 *         vertex count: number of vertices in the group starting after the last
 *                       group ended.
 *         bone index: the index of the bone the group is attached to.
 *
 * ------------------------ start of blend block -------------------------------
 *
 * - 8 bytes * total vertex blend count, each record containing:
 *         indices: 2 16 bit ints representing an index TODO: unknown of what
 *         weight: float representing a normalized weight between 0 and 1
 *
 */

#include "L3DFile.h"

#include <cassert>
#include <cstring>

#include <fstream>
#include <limits>
#include <vector>

using namespace openblack::l3d;

namespace
{
// Adapted from https://stackoverflow.com/a/13059195/10604387
//          and https://stackoverflow.com/a/46069245/10604387
struct membuf: std::streambuf
{
	membuf(char const* base, size_t size)
	{
		char* p(const_cast<char*>(base));
		this->setg(p, p, p + size);
	}
	std::streampos seekoff(off_type off, std::ios_base::seekdir way, [[maybe_unused]] std::ios_base::openmode which) override
	{
		if (way == std::ios_base::cur)
		{
			gbump(static_cast<int>(off));
		}
		else if (way == std::ios_base::end)
		{
			setg(eback(), egptr() + off, egptr());
		}
		else if (way == std::ios_base::beg)
		{
			setg(eback(), eback() + off, egptr());
		}
		return gptr() - eback();
	}

	std::streampos seekpos([[maybe_unused]] pos_type pos, [[maybe_unused]] std::ios_base::openmode which) override
	{
		return seekoff(pos - pos_type(off_type(0)), std::ios_base::beg, which);
	}
};
struct imemstream: virtual membuf, std::istream
{
	imemstream(char const* base, size_t size)
	    : membuf(base, size)
	    , std::istream(dynamic_cast<std::streambuf*>(this))
	{
	}
};
} // namespace

template <typename Item>
void add_span(std::vector<std::span<Item>>& container, typename std::vector<Item>& items, size_t offset, size_t length)
{
	if (length > 0)
	{
		container.emplace_back(&items[offset], length);
	}
	else
	{
		// Add an empty span
		container.emplace_back();
	}
}

/// Error handling
void L3DFile::Fail(const std::string& msg)
{
	throw std::runtime_error("L3D Error: " + msg + "\nFilename: " + _filename.string());
}

L3DFile::L3DFile() = default;
L3DFile::~L3DFile() = default;

void L3DFile::ReadFile(std::istream& stream)
{
	assert(!_isLoaded);

	// Total file size
	std::size_t fsize = 0;
	if (stream.seekg(0, std::ios_base::end))
	{
		fsize = static_cast<std::size_t>(stream.tellg());
		stream.seekg(0);
	}

	if (fsize < sizeof(L3DHeader))
	{
		Fail("File too small to be a valid L3D file.");
	}

	// First 76 bytes
	stream.read(reinterpret_cast<char*>(&_header), sizeof(L3DHeader));
	if (_header.magic != k_Magic)
	{
		Fail("Unrecognized L3D header");
	}

	// Read the offset info into a temporary buffers
	std::vector<uint32_t> submeshOffsets(_header.submeshCount);
	if (!submeshOffsets.empty() && _header.submeshOffsetsOffset != std::numeric_limits<uint32_t>::max())
	{
		if (_header.submeshOffsetsOffset > fsize)
		{
			Fail("Submesh Offset is beyond the size of the file");
		}
		if (_header.submeshOffsetsOffset + submeshOffsets.size() * sizeof(submeshOffsets[0]) > fsize)
		{
			Fail("Submesh Offsets are beyond the end of the file");
		}
		stream.seekg(_header.submeshOffsetsOffset);
		stream.read(reinterpret_cast<char*>(submeshOffsets.data()), submeshOffsets.size() * sizeof(submeshOffsets[0]));
	}
	std::vector<uint32_t> skinOffsets;
	skinOffsets.resize(_header.skinCount);
	if (!skinOffsets.empty() && _header.skinOffsetsOffset != std::numeric_limits<uint32_t>::max())
	{
		if (_header.skinOffsetsOffset > fsize)
		{
			Fail("Skin Offset is beyond the size of the file");
		}
		if (_header.skinOffsetsOffset + skinOffsets.size() * sizeof(skinOffsets[0]) > fsize)
		{
			Fail("Skin Offsets are beyond the end of the file");
		}
		stream.seekg(_header.skinOffsetsOffset);
		stream.read(reinterpret_cast<char*>(skinOffsets.data()), skinOffsets.size() * sizeof(skinOffsets[0]));
	}
	_extraPoints.resize(_header.extraDataCount);
	if (!_extraPoints.empty() && _header.extraDataOffset != std::numeric_limits<uint32_t>::max())
	{
		if (_header.extraDataOffset > fsize)
		{
			Fail("Point Offset is beyond the size of the file");
		}
		if (_header.extraDataOffset + _extraPoints.size() * sizeof(_extraPoints[0]) > fsize)
		{
			Fail("Points are beyond the end of the file");
		}
		stream.seekg(_header.extraDataOffset);
		stream.read(reinterpret_cast<char*>(_extraPoints.data()), _extraPoints.size() * sizeof(_extraPoints[0]));
	}

	// Reserve space and read submeshes
	uint32_t totalPrimitives = 0;
	uint32_t totalBones = 0;
	_submeshHeaders.reserve(submeshOffsets.size());
	for (auto offset : submeshOffsets)
	{
		if (offset + sizeof(_submeshHeaders[0]) > fsize)
		{
			Fail("Submesh header is beyond the end of the file");
		}
		stream.seekg(offset);
		auto& header = _submeshHeaders.emplace_back();
		stream.read(reinterpret_cast<char*>(&header), sizeof(header));
		totalPrimitives += header.numPrimitives;
		totalBones += header.numBones;
	}

	// Reserve space and read skins
	_skins.reserve(skinOffsets.size());
	for (auto offset : skinOffsets)
	{
		stream.seekg(offset);
		auto& skin = _skins.emplace_back();
		stream.read(reinterpret_cast<char*>(&skin), sizeof(skin));
	}

	// Reserve space for primitive offsets
	std::vector<uint32_t> primitiveOffsets(totalPrimitives);
	uint32_t primitiveCounter = 0;
	for (const auto& header : _submeshHeaders)
	{
		stream.seekg(header.primitivesOffset);
		if (primitiveCounter + header.numPrimitives > totalPrimitives)
		{
			Fail("More primitives found than declared");
		}

		if (header.numPrimitives == 0)
		{
			continue;
		}
		if (primitiveOffsets.size() <= primitiveCounter)
		{
			Fail("Less Primitives than expected");
		}
		if (primitiveOffsets[primitiveCounter] > fsize)
		{
			Fail("Primitive Offset is beyond the size of the file");
		}
		if (primitiveOffsets[primitiveCounter] + primitiveOffsets[primitiveCounter] * sizeof(primitiveOffsets[0]) > fsize)
		{
			Fail("Primitive Offsets are beyond the end of the file");
		}
		stream.read(reinterpret_cast<char*>(&primitiveOffsets[primitiveCounter]),
		            header.numPrimitives * sizeof(primitiveOffsets[0]));
		primitiveCounter += header.numPrimitives;
	}
	if (primitiveCounter != totalPrimitives)
	{
		Fail("Could not account for all primitives");
	}

	// Reserve space for primitives
	_primitiveHeaders.reserve(primitiveOffsets.size());
	uint32_t totalVertices = 0;
	uint32_t totalIndices = 0;
	uint32_t totalGroups = 0;
	uint32_t totalBlendValues = 0;
	for (auto offset : primitiveOffsets)
	{
		if (offset + sizeof(_primitiveHeaders[0]) > fsize)
		{
			Fail("Primitive headers are beyond the end of the file");
		}
		stream.seekg(offset);
		_primitiveHeaders.emplace_back();
		stream.read(reinterpret_cast<char*>(&_primitiveHeaders.back()), sizeof(_primitiveHeaders[0]));
		totalVertices += _primitiveHeaders.back().numVertices;
		totalIndices += _primitiveHeaders.back().numTriangles * 3;
		totalGroups += _primitiveHeaders.back().numGroups;
		totalBlendValues += _primitiveHeaders.back().numVertexBlends;
	}

	// Reserve space for vertices
	_vertices.resize(totalVertices);
	if (!_vertices.empty())
	{
		uint32_t counter = 0;
		for (const auto& header : _primitiveHeaders)
		{
			if (header.numVertices == 0 || header.verticesOffset == std::numeric_limits<uint32_t>::max())
			{
				continue;
			}
			if (header.verticesOffset + header.numVertices * sizeof(_vertices[0]) > fsize)
			{
				Fail("Vertex list go beyond file");
			}
			stream.seekg(header.verticesOffset);
			stream.read(reinterpret_cast<char*>(&_vertices[counter]), header.numVertices * sizeof(_vertices[0]));
			counter += header.numVertices;
		}
		if (counter != totalVertices)
		{
			Fail("Could not account for all vertices");
		}
	}

	// Reserve space for indices
	_indices.resize(totalIndices);
	if (!_indices.empty())
	{
		uint32_t counter = 0;
		for (const auto& header : _primitiveHeaders)
		{
			if (header.numTriangles == 0 || header.trianglesOffset == std::numeric_limits<uint32_t>::max())
			{
				continue;
			}
			if (header.trianglesOffset + header.numTriangles * 3 * sizeof(_indices[0]) > fsize)
			{
				Fail("Triangle list go beyond file");
			}
			stream.seekg(header.trianglesOffset);
			stream.read(reinterpret_cast<char*>(&_indices[counter]), header.numTriangles * 3 * sizeof(_indices[0]));
			counter += header.numTriangles * 3;
		}
		if (counter != totalIndices)
		{
			Fail("Could not account for all indices");
		}
	}

	// Reserve space for look-up table data
	_vertexGroups.resize(totalGroups);
	if (!_vertexGroups.empty())
	{
		uint32_t counter = 0;
		for (const auto& header : _primitiveHeaders)
		{
			if (header.numGroups == 0 || header.groupsOffset == std::numeric_limits<uint32_t>::max())
			{
				continue;
			}
			if (header.groupsOffset + header.numGroups * sizeof(_vertexGroups[0]) > fsize)
			{
				Fail("Vertex groups go beyond end of file");
			}
			stream.seekg(header.groupsOffset);
			stream.read(reinterpret_cast<char*>(&_vertexGroups[counter]), header.numGroups * sizeof(_vertexGroups[0]));
			counter += header.numGroups;
		}
		if (counter != totalGroups)
		{
			Fail("Could not account for vertex group data");
		}
	}

	// Reserve space for vertex blend data
	_blends.resize(totalBlendValues);

	if (!_blends.empty())
	{
		uint32_t counter = 0;
		for (const auto& header : _primitiveHeaders)
		{
			if (header.numVertexBlends == 0 || header.vertexBlendsOffset == std::numeric_limits<uint32_t>::max())
			{
				continue;
			}
			if (header.vertexBlendsOffset + header.numVertexBlends * sizeof(_blends[0]) > fsize)
			{
				Fail("Blend value data goes beyond file");
			}
			stream.seekg(header.vertexBlendsOffset);
			stream.read(reinterpret_cast<char*>(&_blends[counter]), header.numVertexBlends * sizeof(_blends[0]));
			counter += header.numVertexBlends;
		}
		if (counter != totalBlendValues)
		{
			Fail("Could not account for blend values data");
		}
	}

	// Reserve space for bone data
	_bones.resize(totalBones);
	if (!_bones.empty())
	{
		uint32_t counter = 0;
		for (const auto& header : _submeshHeaders)
		{
			if (header.numBones == 0 || header.bonesOffset == std::numeric_limits<uint32_t>::max())
			{
				continue;
			}
			if (header.bonesOffset + header.numBones * sizeof(_bones[0]) > fsize)
			{
				Fail("Bone value data goes beyond file");
			}
			stream.seekg(header.bonesOffset);
			stream.read(reinterpret_cast<char*>(&_bones[counter]), header.numBones * sizeof(_bones[0]));
			counter += header.numBones;
		}
		if (counter != totalBones)
		{
			Fail("Could not account for bone values data");
		}
	}

	// Get additional data. Strictly in this order
	// Footprint data
	uint32_t additionalDataOffset = 0;
	uint32_t footprintDataSize = 0;
	if ((static_cast<uint32_t>(_header.flags) & static_cast<uint32_t>(L3DMeshFlags::ContainsLandscapeFeature)) != 0u)
	{
		// TODO(#484): Investigate optional footprint block
		stream.seekg(0x48, std::istream::beg);
		stream.read(reinterpret_cast<char*>(&additionalDataOffset), sizeof(additionalDataOffset));
		stream.seekg(additionalDataOffset + 8, std::istream::beg);
		stream.read(reinterpret_cast<char*>(&footprintDataSize), sizeof(footprintDataSize));
		auto footprintData = std::vector<int16_t>(footprintDataSize / sizeof(int16_t));
		stream.read(reinterpret_cast<char*>(footprintData.data()), footprintDataSize);
		_footprintData.resize(footprintDataSize);
		stream.read(reinterpret_cast<char*>(_footprintData.data()), _footprintData.size());
	}

	// UV2 data
	uint32_t uv2DataSize = 0;
	if ((static_cast<uint32_t>(_header.flags) & static_cast<uint32_t>(L3DMeshFlags::ContainsUV2)) != 0u)
	{
		// TODO(#483): Investigate optional UV2 block
		stream.seekg(0x48, std::istream::beg);
		stream.read(reinterpret_cast<char*>(&additionalDataOffset), sizeof(additionalDataOffset));
		stream.seekg(additionalDataOffset + footprintDataSize, std::istream::beg);
		stream.read(reinterpret_cast<char*>(&uv2DataSize), sizeof(uv2DataSize));
		stream.seekg(8, std::istream::cur);
		_uv2Data.resize(uv2DataSize);
		stream.read(reinterpret_cast<char*>(_uv2Data.data()), _uv2Data.size());
	}

	// Name data
	uint32_t nameDataSize = 0;
	if ((static_cast<uint32_t>(_header.flags) & static_cast<uint32_t>(L3DMeshFlags::ContainsNameData)) != 0u)
	{
		stream.seekg(0x48, std::istream::beg);
		stream.read(reinterpret_cast<char*>(&additionalDataOffset), sizeof(additionalDataOffset));
		stream.seekg(additionalDataOffset + footprintDataSize + uv2DataSize, std::istream::beg);
		stream.read(reinterpret_cast<char*>(&nameDataSize), sizeof(nameDataSize));
		stream.seekg(8, std::istream::cur);
		_nameData.resize(nameDataSize);
		stream.read(_nameData.data(), _nameData.size());
	}

	// Create spans per submesh
	_primitiveSpans.reserve(_submeshHeaders.size());
	_boneSpans.reserve(_submeshHeaders.size());
	{
		uint32_t primitiveStart = 0;
		uint32_t boneStart = 0;
		for (auto& submeshHeader : _submeshHeaders)
		{
			add_span(_primitiveSpans, _primitiveHeaders, primitiveStart, submeshHeader.numPrimitives);
			add_span(_boneSpans, _bones, boneStart, submeshHeader.numBones);
			primitiveStart += submeshHeader.numPrimitives;
			boneStart += submeshHeader.numBones;
		}
	}

	// Create Primitive spans per submesh
	_vertexSpans.reserve(_submeshHeaders.size());
	_indexSpans.reserve(_submeshHeaders.size());
	_vertexGroupSpans.reserve(_submeshHeaders.size());
	{
		uint32_t vertexStart = 0;
		uint32_t indexStart = 0;
		uint32_t vertexGroupStart = 0;
		for (uint32_t i = 0; i < _submeshHeaders.size(); ++i)
		{
			uint32_t vertexLength = 0;
			uint32_t indexLength = 0;
			uint32_t vertexGroupLength = 0;
			for (auto& primitive : GetPrimitiveSpan(i))
			{
				vertexLength += primitive.numVertices;
				indexLength += primitive.numTriangles * 3;
				vertexGroupLength += primitive.numGroups;
			}

			add_span(_vertexSpans, _vertices, vertexStart, vertexLength);
			add_span(_indexSpans, _indices, indexStart, indexLength);
			add_span(_vertexGroupSpans, _vertexGroups, vertexGroupStart, vertexGroupLength);
			vertexStart += vertexLength;
			indexStart += indexLength;
			vertexGroupStart += vertexGroupLength;
		}
	}

	_isLoaded = true;
}

void L3DFile::WriteFile(std::ostream& stream) const
{
	assert(!_isLoaded);

	size_t submeshOffsetsBase = sizeof(L3DHeader);
	size_t skinOffsetsBase = submeshOffsetsBase + _header.submeshCount * sizeof(uint32_t);
	size_t pointsBase = skinOffsetsBase + _header.skinCount * sizeof(uint32_t);
	size_t submeshBase = pointsBase + _header.extraDataCount * sizeof(_extraPoints[0]);
	size_t skinBase = submeshBase + _header.submeshCount * sizeof(_submeshHeaders[0]);
	size_t primitiveOffsetBase = skinBase + _header.skinCount * sizeof(_skins[0]);
	size_t primitiveBase = primitiveOffsetBase + _primitiveHeaders.size() * sizeof(uint32_t);

	std::vector<uint32_t> submeshOffsets;
	submeshOffsets.resize(_header.submeshCount);
	for (uint32_t i = 0; i < _header.submeshCount; ++i)
	{
		submeshOffsets[i] = static_cast<uint32_t>(submeshBase);
		submeshBase += sizeof(_submeshHeaders[0]);
	}
	std::vector<uint32_t> skinOffsets;
	skinOffsets.resize(_header.skinCount);
	for (uint32_t i = 0; i < _header.skinCount; ++i)
	{
		skinOffsets[i] = static_cast<uint32_t>(skinBase);
		skinOffsetsBase += sizeof(_skins[0]);
	}
	uint32_t totalPrimitives = 0;
	for (const auto& header : _submeshHeaders)
	{
		totalPrimitives += header.numPrimitives;
	}
	std::vector<uint32_t> primitiveOffsets;
	primitiveOffsets.resize(totalPrimitives);
	uint32_t currentPrimitive = 0;
	for (size_t i = 0; i < _submeshHeaders.size(); ++i)
	{
		for (size_t j = 0; j < _primitiveSpans[i].size(); ++j)
		{
			primitiveOffsets[currentPrimitive] =
			    static_cast<uint32_t>(primitiveBase + currentPrimitive * sizeof(_primitiveHeaders[0]));
			currentPrimitive++;
		}
	}

	stream.write(reinterpret_cast<const char*>(&_header), sizeof(L3DHeader));
	if (!submeshOffsets.empty())
	{
		stream.write(reinterpret_cast<const char*>(submeshOffsets.data()), submeshOffsets.size() * sizeof(submeshOffsets[0]));
	}
	if (!skinOffsets.empty())
	{
		stream.write(reinterpret_cast<const char*>(skinOffsets.data()), skinOffsets.size() * sizeof(skinOffsets[0]));
	}
	if (!_extraPoints.empty())
	{
		stream.write(reinterpret_cast<const char*>(_extraPoints.data()), _extraPoints.size() * sizeof(_extraPoints[0]));
	}
	if (!_submeshHeaders.empty())
	{
		stream.write(reinterpret_cast<const char*>(_submeshHeaders.data()),
		             _submeshHeaders.size() * sizeof(_submeshHeaders[0]));
	}
	if (!_skins.empty())
	{
		stream.write(reinterpret_cast<const char*>(_skins.data()), _skins.size() * sizeof(_skins[0]));
	}
	if (!primitiveOffsets.empty())
	{
		stream.write(reinterpret_cast<const char*>(primitiveOffsets.data()),
		             primitiveOffsets.size() * sizeof(primitiveOffsets[0]));
	}
	if (!_primitiveHeaders.empty())
	{
		stream.write(reinterpret_cast<const char*>(_primitiveHeaders.data()),
		             _primitiveHeaders.size() * sizeof(_primitiveHeaders[0]));
	}
	if (!_bones.empty())
	{
		stream.write(reinterpret_cast<const char*>(_bones.data()), _bones.size() * sizeof(_bones[0]));
	}
	if (!_vertices.empty())
	{
		stream.write(reinterpret_cast<const char*>(_vertices.data()), _vertices.size() * sizeof(_vertices[0]));
	}
	if (!_indices.empty())
	{
		stream.write(reinterpret_cast<const char*>(_indices.data()), _indices.size() * sizeof(_indices[0]));
	}
	if (!_vertexGroups.empty())
	{
		stream.write(reinterpret_cast<const char*>(_vertexGroups.data()), _vertexGroups.size() * sizeof(_vertexGroups[0]));
	}
	if (!_blends.empty())
	{
		stream.write(reinterpret_cast<const char*>(_blends.data()), _blends.size() * sizeof(_blends[0]));
	}
}

void L3DFile::Open(const std::filesystem::path& filepath)
{
	assert(!_isLoaded);

	_filename = filepath;

	std::ifstream stream(_filename, std::ios::binary);

	if (!stream.is_open())
	{
		Fail("Could not open file.");
	}

	ReadFile(stream);
}

void L3DFile::Open(const std::vector<uint8_t>& buffer)
{
	assert(!_isLoaded);

	imemstream stream(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(buffer[0]));

	// File name set to "buffer" when file is load from a buffer
	// Impact code using L3DFile::GetFilename method
	_filename = std::filesystem::path("buffer");

	ReadFile(stream);
}

void L3DFile::Write(const std::filesystem::path& filepath)
{
	assert(!_isLoaded);

	_filename = filepath;

	std::ofstream stream(_filename, std::ios::binary);

	if (!stream.is_open())
	{
		Fail("Could not open file.");
	}

	// Set magic number
	_header.magic = k_Magic;
	_header.submeshCount = static_cast<uint32_t>(_submeshHeaders.size());
	_header.skinCount = static_cast<uint32_t>(_skins.size());
	_header.extraDataCount = static_cast<uint32_t>(_extraPoints.size());
	size_t offset = sizeof(_header);
	_header.submeshOffsetsOffset =
	    _header.submeshCount > 0 ? static_cast<uint32_t>(offset) : std::numeric_limits<uint32_t>::max();
	offset += _header.submeshCount * sizeof(uint32_t);
	_header.anotherOffset = std::numeric_limits<uint32_t>::max();
	_header.skinOffsetsOffset = _header.skinCount > 0 ? static_cast<uint32_t>(offset) : std::numeric_limits<uint32_t>::max();
	offset += _header.skinCount * sizeof(uint32_t);
	_header.extraDataOffset = _header.extraDataCount > 0 ? static_cast<uint32_t>(offset) : std::numeric_limits<uint32_t>::max();
	// offset += _header.extraDataCount * sizeof(_extraPoints[0]);
	_header.footprintDataOffset = std::numeric_limits<uint32_t>::max();

	size_t submeshOffsetsBase = sizeof(L3DHeader);
	size_t skinOffsetsBase = submeshOffsetsBase + _header.submeshCount * sizeof(uint32_t);
	size_t pointsBase = skinOffsetsBase + _header.skinCount * sizeof(uint32_t);
	size_t submeshBase = pointsBase + _header.extraDataCount * sizeof(_extraPoints[0]);
	size_t skinBase = submeshBase + _submeshHeaders.size() * sizeof(_submeshHeaders[0]);
	size_t primitiveOffsetBase = skinBase + _skins.size() * sizeof(_skins[0]);
	size_t primitiveBase = primitiveOffsetBase + _primitiveHeaders.size() * sizeof(uint32_t);
	size_t boneBase = primitiveBase + _primitiveHeaders.size() * sizeof(_primitiveHeaders[0]);
	size_t vertexBase = boneBase + _bones.size() * sizeof(_bones[0]);
	size_t triangleBase = vertexBase + _vertices.size() * sizeof(_vertices[0]);
	size_t lutBase = triangleBase + _indices.size() * sizeof(_indices[0]);
	size_t blendBase = lutBase + _vertexGroups.size() * sizeof(_vertexGroups[0]);
	_header.size = static_cast<uint32_t>(blendBase + _blends.size() * sizeof(_blends[0]));
	if (!_bones.empty())
	{
		_header.flags =
		    static_cast<L3DMeshFlags>(static_cast<uint32_t>(_header.flags) | static_cast<uint32_t>(L3DMeshFlags::HasBones));
	}

	for (size_t i = 0; i < _primitiveHeaders.size(); ++i)
	{
		_primitiveHeaders[i].verticesOffset =
		    !_vertexSpans[i].empty() ? static_cast<uint32_t>(vertexBase) : std::numeric_limits<uint32_t>::max();
		_primitiveHeaders[i].trianglesOffset =
		    !_indexSpans[i].empty() ? static_cast<uint32_t>(triangleBase) : std::numeric_limits<uint32_t>::max();
		_primitiveHeaders[i].groupsOffset = std::numeric_limits<uint32_t>::max();       // TODO(#468) boneVertLUTBase;
		_primitiveHeaders[i].vertexBlendsOffset = std::numeric_limits<uint32_t>::max(); // TODO(#468) vertexBlendsBase;
		vertexBase += _vertexSpans[i].size() * sizeof(_vertices[0]);
		triangleBase += 3 * _indexSpans[i].size() * sizeof(_indices[0]);
		// TODO(#468)
		//  boneVertLUTBase += 3 * _lookupSpans[i].size() * sizeof(_lookUpTable[0]);
		//  vertexBlendsBase += _blendSpans[i].size() * sizeof(_blends[0]);
	}
	for (auto& header : _submeshHeaders)
	{
		// TODO(#469): Set flags
		header.primitivesOffset =
		    header.numPrimitives != 0u ? static_cast<uint32_t>(primitiveOffsetBase) : std::numeric_limits<uint32_t>::max();
		header.bonesOffset = header.numBones != 0u ? static_cast<uint32_t>(boneBase) : std::numeric_limits<uint32_t>::max();
		primitiveBase += header.numPrimitives * sizeof(L3DPrimitiveHeader);
		boneBase += header.numBones * sizeof(L3DBone);
	}

	WriteFile(stream);
}

void L3DFile::AddSubmesh(const L3DSubmeshHeader& header) noexcept
{
	_submeshHeaders.push_back(header);
}

void L3DFile::AddPrimitives(const std::vector<L3DPrimitiveHeader>& headers) noexcept
{
	auto size = _primitiveHeaders.size();
	for (const auto& header : headers)
	{
		_primitiveHeaders.push_back(header);
	}
	_primitiveSpans.emplace_back(&_primitiveHeaders[size], static_cast<uint32_t>(headers.size()));
}

void L3DFile::AddVertices(const std::vector<L3DVertex>& vertices) noexcept
{
	auto size = _vertices.size();
	for (const auto& vertex : vertices)
	{
		_vertices.push_back(vertex);
	}
	_vertexSpans.emplace_back(&_vertices[static_cast<uint32_t>(size)], static_cast<uint32_t>(vertices.size()));
}

void L3DFile::AddIndices(const std::vector<uint16_t>& indices) noexcept
{
	auto size = _indices.size();
	for (const auto& index : indices)
	{
		_indices.push_back(index);
	}
	_indexSpans.emplace_back(&_indices[static_cast<uint32_t>(size)], static_cast<uint32_t>(indices.size()));
}

void L3DFile::AddBones(const std::vector<L3DBone>& bones) noexcept
{
	auto size = _boneSpans.size();
	for (const auto& bone : bones)
	{
		_bones.push_back(bone);
	}
	_boneSpans.emplace_back(&_bones[static_cast<uint32_t>(size)], static_cast<uint32_t>(bones.size()));
}
