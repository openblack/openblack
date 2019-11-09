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
 * ------------------------ start of skin block --------------------------------
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
 *         look-up table size: size of the bone-vertex look-up table
 *                             (see below)
 *         look-up table start offset: offset in look-up table block
 *                                     (see below)
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
 * ------------------------ start of look-up table block -----------------------
 *
 * - ? bytes * total look-up table size, TODO: contents unknown
 *
 * ------------------------ start of blend block -------------------------------
 *
 * - ? bytes * total vertex blendcount, TODO: contents unknown
 *
 */

#include <l3d_file.h>

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
	imemstream(char const* base, size_t size): membuf(base, size), std::istream(dynamic_cast<std::streambuf*>(this)) {}
};
} // namespace

/// Error handling
void L3DFile::Fail(const std::string& msg)
{
	throw std::runtime_error("L3D Error: " + msg + "\nFilename: " + _filename);
}

void L3DFile::ReadFile(std::istream& stream)
{
	assert(!_isLoaded);

	// Total file size
	size_t fsize = 0;
	if (stream.seekg(0, std::ios_base::end))
	{
		fsize = stream.tellg();
		stream.seekg(0);
	}

	if (fsize < sizeof(L3DHeader))
	{
		Fail("File too small to be a valid L3D file.");
	}

	// First 76 bytes
	stream.read(reinterpret_cast<char*>(&_header), sizeof(L3DHeader));
	constexpr const char kMagic[4] = {'L', '3', 'D', '0'};
	if (std::memcmp(&_header.magic, kMagic, sizeof(_header.magic)) != 0)
	{
		Fail("Unrecognized L3D header");
	}

	// Read the offset info into a temporary buffers
	std::vector<uint32_t> submeshOffsets(_header.submeshCount);
	if (_header.submeshOffsetsOffset != std::numeric_limits<uint32_t>::max())
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
	if (_header.skinOffsetsOffset != std::numeric_limits<uint32_t>::max())
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
	if (_header.pointOffsetsOffset != std::numeric_limits<uint32_t>::max())
	{
		if (_header.pointOffsetsOffset > fsize)
		{
			Fail("Point Offset is beyond the size of the file");
		}
		if (_header.pointOffsetsOffset + _points.size() * sizeof(_points[0]) > fsize)
		{
			Fail("Points are beyond the end of the file");
		}
		stream.seekg(_header.pointOffsetsOffset);
		stream.read(reinterpret_cast<char*>(_points.data()), _points.size() * sizeof(_points[0]));
	}

	// Reserve space and read submeshes
	uint32_t totalPrimitives = 0;
	uint32_t totalBones = 0;
	_submeshHeaders.reserve(submeshOffsets.size());
	for (auto offset : submeshOffsets)
	{
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
	uint32_t totalLookUpTableSize = 0;
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
		totalLookUpTableSize += _primitiveHeaders.back().boneVertLUTSize;
		totalBlendValues += _primitiveHeaders.back().numVertexBlends;
	}

	// Reserve space for vertices
	_vertices.resize(totalVertices);
	{
		uint32_t counter = 0;
		for (const auto& header : _primitiveHeaders)
		{
			if (header.verticesOffset == std::numeric_limits<uint32_t>::max())
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
	{
		uint32_t counter = 0;
		for (const auto& header : _primitiveHeaders)
		{
			if (header.trianglesOffset == std::numeric_limits<uint32_t>::max())
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
	_lookUpTable.resize(totalLookUpTableSize);
	{
		uint32_t counter = 0;
		for (const auto& header : _primitiveHeaders)
		{
			if (header.boneVertLUTOffset == std::numeric_limits<uint32_t>::max())
			{
				continue;
			}
			if (header.boneVertLUTOffset + header.boneVertLUTSize > fsize)
			{
				Fail("Look-up table data goes beyond file");
			}
			stream.seekg(header.boneVertLUTOffset);
			stream.read(reinterpret_cast<char*>(&_lookUpTable[counter]), header.boneVertLUTSize);
			counter += header.boneVertLUTSize;
		}
		if (counter != totalLookUpTableSize)
		{
			Fail("Could not account for look-up table data");
		}
	}

	// Reserve space for vertex blend data
	_blends.resize(totalBlendValues);
	{
		uint32_t counter = 0;
		for (const auto& header : _primitiveHeaders)
		{
			if (header.vertexBlendsOffset == std::numeric_limits<uint32_t>::max())
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
	{
		uint32_t counter = 0;
		for (const auto& header : _submeshHeaders)
		{
			if (header.bonesOffset == std::numeric_limits<uint32_t>::max())
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
	{
		uint32_t vertexStart = 0;
		uint32_t indexStart = 0;
		for (uint32_t i = 0; i < _submeshHeaders.size(); ++i)
		{
			uint32_t vertexLength = 0;
			uint32_t indexLength = 0;
			for (auto& primitive : GetPrimitiveSpan(i))
			{
				vertexLength += primitive.numVertices;
				indexLength += primitive.numTriangles * 3;
			}

			_vertexSpans.emplace_back(_vertices, vertexStart, vertexLength);
			_indexSpans.emplace_back(_indices, indexStart, indexLength);

			vertexStart += vertexLength;
			indexStart += indexLength;
		}
	}

	_isLoaded = true;
}

L3DFile::L3DFile() : _isLoaded(false) {}

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
