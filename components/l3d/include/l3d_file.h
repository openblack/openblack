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
 */

#pragma once

#include <array>
#include <string>
#include <vector>

namespace openblack::l3d
{

/**
  This class is used to read L3Ds.
 */
class L3DFile
{
public:
	struct L3DHeader
	{
		char magic[4];
		uint32_t flags;
		uint32_t size;
		uint32_t submeshCount;
		uint32_t submeshOffsetsOffset;
		uint8_t padding[32]; // always zero
		uint32_t anotherOffset;
		uint32_t skinCount;
		uint32_t skinOffsetsOffset;
		uint32_t pointCount;
		uint32_t pointOffsetsOffset;
		uint32_t extraDataOffset;
	};
	static_assert(sizeof(L3DHeader) == 19 * sizeof(uint32_t));

	struct L3DSubmeshHeader
	{
		uint32_t flags;
		uint32_t numPrimitives;
		uint32_t primitivesOffset;
		uint32_t numBones;
		uint32_t bonesOffset;
	};
	static_assert(sizeof(L3DSubmeshHeader) == 5 * sizeof(uint32_t));

	struct L3DTexture
	{
		struct RGBA4
		{
			uint16_t R : 4;
			uint16_t G : 4;
			uint16_t B : 4;
			uint16_t A : 4;
		};
		static_assert(sizeof(RGBA4) == sizeof(uint16_t));
		uint32_t id;
		static constexpr uint16_t width = 256;
		static constexpr uint16_t height = 256;
		std::array<RGBA4, width * height> texels;
		static_assert(sizeof(texels) == width * height * sizeof(uint16_t));
	};
	static_assert(sizeof(L3DTexture) == sizeof(uint32_t) + 256 * 256 * sizeof(uint16_t));

	struct L3DPoint
	{
		float x, y, z;
	};
	static_assert(sizeof(L3DPoint) == 12);
	struct L3DPoint2D
	{
		float x, y;
	};
	static_assert(sizeof(L3DPoint2D) == 8);

	struct L3DBone
	{
		uint32_t parent;
		uint32_t firstChild;
		uint32_t rightSibling;
		float orientation[9];
		L3DPoint position;
	};
	static_assert(sizeof(L3DBone) == 3 * sizeof(uint32_t) + 9 * sizeof(float) + sizeof(L3DPoint));

	struct L3DPrimitiveHeader
	{
		uint32_t unknown_1;
		uint32_t unknown_2;
		uint32_t skinID;
		uint32_t unknown_3;
		uint32_t numVertices;
		uint32_t verticesOffset;
		uint32_t numTriangles;
		uint32_t trianglesOffset;
		uint32_t boneVertLUTSize;
		uint32_t boneVertLUTOffset;
		uint32_t numVertexBlends;
		uint32_t vertexBlendsOffset;
	};
	static_assert(sizeof(L3DPrimitiveHeader) == 12 * sizeof(uint32_t));

	struct L3DVertex
	{
		L3DPoint position;
		L3DPoint2D texCoord;
		L3DPoint normal;
	};
	static_assert(sizeof(L3DVertex) == 32);

protected:
	/// True when a file has been loaded
	bool _isLoaded;

	std::string _filename;

	L3DHeader _header;
	std::vector<L3DSubmeshHeader> _submeshHeaders;
	std::vector<L3DTexture> _skins;
	std::vector<L3DPoint> _points;
	std::vector<L3DPrimitiveHeader> _primitiveHeaders;
	std::vector<L3DVertex> _vertices;
	std::vector<uint16_t> _indices;
	std::vector<uint8_t> _lookUpTable;
	std::vector<uint8_t> _blends;
	std::vector<L3DBone> _bones;

	/// Error handling
	void Fail(const std::string& msg);

	/// Read file from the input source
	virtual void ReadFile(std::istream& stream);

public:
	L3DFile();
	virtual ~L3DFile() = default;

	void Open(const std::string& file);
	void Open(const std::vector<uint8_t>& buffer);

	[[nodiscard]] const std::string& GetFilename() const { return _filename; }
	[[nodiscard]] const L3DHeader& GetHeader() const { return _header; }
	[[nodiscard]] const std::vector<L3DSubmeshHeader>& GetSubmeshHeaders() const { return _submeshHeaders; }
	[[nodiscard]] const std::vector<L3DTexture>& GetSkins() const { return _skins; }
	[[nodiscard]] const std::vector<L3DPoint>& GetPoints() const { return _points; }
	[[nodiscard]] const std::vector<L3DPrimitiveHeader>& GetPrimitiveHeaders() const { return _primitiveHeaders; }
	[[nodiscard]] const std::vector<L3DVertex>& GetVertices() const { return _vertices; }
	[[nodiscard]] const std::vector<uint16_t>& GetIndices() const { return _indices; }
	[[nodiscard]] const std::vector<uint8_t>& GetLookUpTableData() const { return _lookUpTable; }
	[[nodiscard]] const std::vector<uint8_t>& GetBlends() const { return _blends; }
	[[nodiscard]] const std::vector<L3DBone>& GetBones() const { return _bones; }
};

} // namespace openblack::l3d
