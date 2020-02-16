/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 *
 * The layout of a L3D File is as follows:
 *
 * - 76 byte header, containing 19 ints
 *         magic number, containing 4 chars "L3D0"
 *         flags
 *         submesh count - count of the meshes in submesh block (see below)
 *         submesh start offset - offset of the submesh block (see below)
 *         unknown 8 empty int - all zeros
 *         unknown offset
 *         skin count - count of the skins in skin block (see below)
 *         skin start offset - offset of the skin block (see below)
 *         point count - count of the points in point block (see below)
 *         point start offset - offset of the point block (see below)
 *         extra data offset - offset of the extra data block (see below)
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
 * ------------------------ start of point block -------------------------------
 *
 *  TODO: Validate me
 * - 12 bytes * total points, each containing point data as floats for x, y, z
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
 *         unknown 1
 *         unknown 2
 *         skin id: id of the skin to use (see above)
 *         unknown 3
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

#include <L3DFile.h>

#include <cassert>
#include <cstring>
#include <fstream>
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
	std::streampos seekoff(off_type off, std::ios_base::seekdir way,
	                       [[maybe_unused]] std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override
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

	std::streampos seekpos([[maybe_unused]] pos_type pos,
	                       [[maybe_unused]] std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override
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

/// Error handling
void L3DFile::Fail(const std::string& msg)
{
	throw std::runtime_error("L3D Error: " + msg + "\nFilename: " + _filename);
}

L3DFile::L3DFile()
    : _isLoaded(false)
{
}

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
	if (std::memcmp(&_header.magic, kMagic, sizeof(_header.magic)) != 0)
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
	_points.resize(_header.pointCount);
	if (!_points.empty() && _header.pointOffset != std::numeric_limits<uint32_t>::max())
	{
		if (_header.pointOffset > fsize)
		{
			Fail("Point Offset is beyond the size of the file");
		}
		if (_header.pointOffset + _points.size() * sizeof(_points[0]) > fsize)
		{
			Fail("Points are beyond the end of the file");
		}
		stream.seekg(_header.pointOffset);
		stream.read(reinterpret_cast<char*>(_points.data()), _points.size() * sizeof(_points[0]));
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
		_submeshHeaders.emplace_back();
		stream.read(reinterpret_cast<char*>(&_submeshHeaders.back()), sizeof(_submeshHeaders[0]));
		totalPrimitives += _submeshHeaders.back().numPrimitives;
		totalBones += _submeshHeaders.back().numBones;
	}

	// Reserve space and read skins
	_skins.reserve(skinOffsets.size());
	for (auto offset : skinOffsets)
	{
		stream.seekg(offset);
		_skins.emplace_back();
		stream.read(reinterpret_cast<char*>(&_skins.back()), sizeof(_skins[0]));
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

	// Create spans per submesh
	_primitiveSpans.reserve(_submeshHeaders.size());
	_boneSpans.reserve(_submeshHeaders.size());
	{
		uint32_t primitiveStart = 0;
		uint32_t boneStart = 0;
		for (auto& _submeshHeader : _submeshHeaders)
		{
			_primitiveSpans.emplace_back(_primitiveHeaders, primitiveStart, _submeshHeader.numPrimitives);
			primitiveStart += _submeshHeader.numPrimitives;
			_boneSpans.emplace_back(_bones, boneStart, _submeshHeader.numBones);
			boneStart += _submeshHeader.numBones;
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

			_vertexSpans.emplace_back(_vertices, vertexStart, vertexLength);
			_indexSpans.emplace_back(_indices, indexStart, indexLength);
			_vertexGroupSpans.emplace_back(_vertexGroups, vertexGroupStart, vertexGroupLength);

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
	size_t submeshBase = pointsBase + _header.pointCount * sizeof(_points[0]);
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
	for (auto& header : _submeshHeaders)
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
	if (!_points.empty())
	{
		stream.write(reinterpret_cast<const char*>(_points.data()), _points.size() * sizeof(_points[0]));
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

void L3DFile::Open(const std::string& file)
{
	assert(!_isLoaded);

	_filename = file;

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

	_filename = "buffer";

	ReadFile(stream);
}

void L3DFile::Write(const std::string& file)
{
	assert(!_isLoaded);

	_filename = file;

	std::ofstream stream(_filename, std::ios::binary);

	if (!stream.is_open())
	{
		Fail("Could not open file.");
	}

	// Set magic number
	std::memcpy(_header.magic, kMagic, sizeof(kMagic));
	_header.submeshCount = static_cast<uint32_t>(_submeshHeaders.size());
	_header.skinCount = static_cast<uint32_t>(_skins.size());
	_header.pointCount = static_cast<uint32_t>(_points.size());
	size_t offset = sizeof(_header);
	_header.submeshOffsetsOffset =
	    _header.submeshCount > 0 ? static_cast<uint32_t>(offset) : std::numeric_limits<uint32_t>::max();
	offset += _header.submeshCount * sizeof(uint32_t);
	_header.anotherOffset = std::numeric_limits<uint32_t>::max();
	_header.skinOffsetsOffset = _header.skinCount > 0 ? static_cast<uint32_t>(offset) : std::numeric_limits<uint32_t>::max();
	offset += _header.skinCount * sizeof(uint32_t);
	_header.pointOffset = _header.pointCount > 0 ? static_cast<uint32_t>(offset) : std::numeric_limits<uint32_t>::max();
	offset += _header.pointCount * sizeof(_points[0]);
	_header.extraDataOffset = std::numeric_limits<uint32_t>::max();

	size_t submeshOffsetsBase = sizeof(L3DHeader);
	size_t skinOffsetsBase = submeshOffsetsBase + _header.submeshCount * sizeof(uint32_t);
	size_t pointsBase = skinOffsetsBase + _header.skinCount * sizeof(uint32_t);
	size_t submeshBase = pointsBase + _header.pointCount * sizeof(_points[0]);
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
		    _vertexSpans[i].size() > 0 ? static_cast<uint32_t>(vertexBase) : std::numeric_limits<uint32_t>::max();
		_primitiveHeaders[i].trianglesOffset =
		    _indexSpans[i].size() > 0 ? static_cast<uint32_t>(triangleBase) : std::numeric_limits<uint32_t>::max();
		_primitiveHeaders[i].groupsOffset = std::numeric_limits<uint32_t>::max();       // TODO boneVertLUTBase;
		_primitiveHeaders[i].vertexBlendsOffset = std::numeric_limits<uint32_t>::max(); // TODO vertexBlendsBase;
		vertexBase += _vertexSpans[i].size() * sizeof(_vertices[0]);
		triangleBase += 3 * _indexSpans[i].size() * sizeof(_indices[0]);
		// TODO
		//  boneVertLUTBase += 3 * _lookupSpans[i].size() * sizeof(_lookUpTable[0]);
		//  vertexBlendsBase += _blendSpans[i].size() * sizeof(_blends[0]);
	}
	for (auto& header : _submeshHeaders)
	{
		// TODO Set flags
		header.primitivesOffset =
		    header.numPrimitives ? static_cast<uint32_t>(primitiveOffsetBase) : std::numeric_limits<uint32_t>::max();
		header.bonesOffset = header.numBones ? static_cast<uint32_t>(boneBase) : std::numeric_limits<uint32_t>::max();
		primitiveBase += header.numPrimitives * sizeof(L3DPrimitiveHeader);
		boneBase += header.numBones * sizeof(L3DBone);
	}

	WriteFile(stream);
}

void L3DFile::AddSubmesh(const L3DSubmeshHeader& header)
{
	_submeshHeaders.push_back(header);
}

void L3DFile::AddPrimitives(const std::vector<L3DPrimitiveHeader>& headers)
{
	_primitiveSpans.emplace_back(_primitiveHeaders, static_cast<uint32_t>(_primitiveHeaders.size()),
	                             static_cast<uint32_t>(headers.size()));
	for (auto& header : headers)
	{
		_primitiveHeaders.push_back(header);
	}
}

void L3DFile::AddVertices(const std::vector<L3DVertex>& vertices)
{
	_vertexSpans.emplace_back(_vertices, static_cast<uint32_t>(_vertices.size()), static_cast<uint32_t>(vertices.size()));
	for (auto& vertex : vertices)
	{
		_vertices.push_back(vertex);
	}
}

void L3DFile::AddIndices(const std::vector<uint16_t>& indices)
{
	_indexSpans.emplace_back(_indices, static_cast<uint32_t>(_indices.size()), static_cast<uint32_t>(indices.size()));
	for (auto& index : indices)
	{
		_indices.push_back(index);
	}
}

void L3DFile::AddBones(const std::vector<L3DBone>& bones)
{
	_boneSpans.emplace_back(_bones, static_cast<uint32_t>(_bones.size()), static_cast<uint32_t>(bones.size()));
	for (auto& bone : bones)
	{
		_bones.push_back(bone);
	}
}
