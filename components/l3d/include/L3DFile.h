/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <array>
#include <span>
#include <string>
#include <vector>

namespace openblack::l3d
{

enum class L3DMeshFlags : uint32_t
{
	Unknown1 = 1U << 0U,                  // 0x1      (31)
	Unknown2 = 1U << 1U,                  // 0x2      (30)
	Unknown3 = 1U << 2U,                  // 0x4      (29)
	Unknown4 = 1U << 3U,                  // 0x8      (28)
	Unknown5 = 1U << 4U,                  // 0x10     (27)
	Unknown6 = 1U << 5U,                  // 0x20     (26)
	Unknown7 = 1U << 6U,                  // 0x40     (25)
	Unknown8 = 1U << 7U,                  // 0x80     (24)
	HasBones = 1U << 8U,                  // 0x100    (23)
	Unknown10 = 1U << 9U,                 // 0x200    (22)
	Unknown11 = 1U << 10U,                // 0x400    (21)
	HasDoorPosition = 1U << 11U,          // 0x800    (20)
	Packed = 1U << 12U,                   // 0x1000   (19)
	NoDraw = 1U << 13U,                   // 0x2000   (18)
	Unknown15 = 1U << 14U,                // 0x4000   (17)
	ContainsLandscapeFeature = 1U << 15U, // 0x8000   (16)
	Unknown17 = 1U << 16U,                // 0x10000  (15)
	Unknown18 = 1U << 17U,                // 0x20000  (14)
	ContainsUV2 = 1U << 18U,              // 0x40000  (13)
	ContainsNameData = 1U << 19U,         // 0x80000  (12)
	ContainsExtraMetrics = 1U << 20U,     // 0x100000 (11)
	ContainsEBone = 1U << 21U,            // 0x200000 (10)
	ContainsTnLData = 1U << 22U,          // 0x400000 (9)
	ContainsNewEP = 1U << 23U,            // 0x800000 (8)
	Unknown25 = 1U << 24U,                // 0x1000000 (7)
	Unknown26 = 1U << 25U,                // 0x2000000 (6)
	Unknown27 = 1U << 26U,                // 0x4000000 (5)
	Unknown28 = 1U << 27U,                // 0x8000000 (4)
	Unknown29 = 1U << 28U,                // 0x10000000 (3)
	Unknown30 = 1U << 29U,                // 0x20000000 (2)
	Unknown31 = 1U << 30U,                // 0x40000000 (1)
	Unknown32 = 1U << 30U,                // 0x80000000 (0)
};

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

struct L3DBoundingBox
{
	uint32_t unknown;
	L3DPoint centre;
	L3DPoint size;
	float diagonalLength;
};
static_assert(sizeof(L3DBoundingBox) == 0x20);

struct L3DHeader
{
	std::array<char, 4> magic;
	L3DMeshFlags flags;
	uint32_t size;
	uint32_t submeshCount;
	uint32_t submeshOffsetsOffset;
	L3DBoundingBox boundingBox; // always zero
	uint32_t anotherOffset;
	uint32_t skinCount;
	uint32_t skinOffsetsOffset;
	uint32_t extraDataCount;
	uint32_t extraDataOffset;
	uint32_t footprintDataOffset;
};
static_assert(sizeof(L3DHeader) == 19 * sizeof(uint32_t));

struct L3DSubmeshHeader
{
	// TODO(bwrsandman): Move to l3d lib
#pragma pack(push, 1)
	struct alignas(4) Flags
	{
		uint32_t hasBones : 1;
		uint32_t lod : 2;
		uint32_t status : 6;
		uint32_t unknown1 : 3; // always 0b0101
		uint32_t isWindow : 1;
		uint32_t isPhysics : 1;
		uint32_t unknown2 : 16; // always 0, probably padding on 32 bits
	};
#pragma pack(pop)
	static_assert(sizeof(Flags) == 4);

	Flags flags;
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
		uint16_t B : 4;
		uint16_t G : 4;
		uint16_t R : 4;
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

struct L3DBone
{
	uint32_t parent;
	uint32_t firstChild;
	uint32_t rightSibling;
	float orientation[9];
	L3DPoint position;
};
static_assert(sizeof(L3DBone) == 3 * sizeof(uint32_t) + 9 * sizeof(float) + sizeof(L3DPoint));

struct L3DMaterial
{
	enum class Type : uint32_t
	{
		Smooth = 0x0,
		SmoothAlpha = 0x1,
		Textured = 0x2,
		TexturedAlpha = 0x3,
		AlphaTextured = 0x4,
		AlphaTexturedAlpha = 0x5,
		AlphaTexturedAlphaNz = 0x6,
		SmoothAlphaNz = 0x7,
		TexturedAlphaNz = 0x8,
		TexturedChroma = 0x9,
		AlphaTexturedAlphaAdditiveChroma = 0xa,
		AlphaTexturedAlphaAdditiveChromaNz = 0xb,
		AlphaTexturedAlphaAdditive = 0xc,
		AlphaTexturedAlphaAdditiveNz = 0xd,
		TexturedChromaAlpha = 0xf,
		TexturedChromaAlphaNz = 0x10,
		ChromaJustZ = 0x12,

		_Count,
	};
	struct BGRA8
	{
		uint8_t b;
		uint8_t g;
		uint8_t r;
		uint8_t a;
	};
	Type type;
	uint8_t alphaCutoutThreshold;
	uint8_t cullMode;
	uint32_t skinID;
	union
	{
		BGRA8 bgra;
		uint32_t raw;
	} color;
};
static_assert(sizeof(L3DMaterial) == 4 * sizeof(uint32_t));

struct L3DPrimitiveHeader
{
	L3DMaterial material;
	uint32_t numVertices;
	uint32_t verticesOffset;
	uint32_t numTriangles;
	uint32_t trianglesOffset;
	uint32_t numGroups;
	uint32_t groupsOffset;
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

struct L3DVertexGroup
{
	uint16_t vertexCount;
	uint16_t boneIndex;
};
static_assert(sizeof(L3DVertexGroup) == 4);

struct L3DBlend
{
	uint16_t indices[2];
	float weight;
};
static_assert(sizeof(L3DBlend) == 8);

/**
  This class is used to read L3Ds.
 */
class L3DFile
{
protected:
	static constexpr const char kMagic[4] = {'L', '3', 'D', '0'};

	/// True when a file has been loaded
	bool _isLoaded;

	std::string _filename;

	L3DHeader _header;
	std::vector<L3DSubmeshHeader> _submeshHeaders;
	std::vector<L3DTexture> _skins;
	/// If the flag HasDoorPosition is on the first extra point is the door
	std::vector<L3DPoint> _extraPoints;
	std::vector<L3DPrimitiveHeader> _primitiveHeaders;
	std::vector<L3DVertex> _vertices;
	std::vector<uint16_t> _indices;
	std::vector<L3DVertexGroup> _vertexGroups;
	std::vector<L3DBlend> _blends;
	std::vector<L3DBone> _bones;
	std::vector<std::span<L3DPrimitiveHeader>> _primitiveSpans;
	std::vector<std::span<L3DVertex>> _vertexSpans;
	std::vector<std::span<uint16_t>> _indexSpans;
	std::vector<std::span<L3DVertexGroup>> _vertexGroupSpans;
	std::vector<std::span<L3DBone>> _boneSpans;

	/// Error handling
	void Fail(const std::string& msg);

	/// Read file from the input source
	virtual void ReadFile(std::istream& stream);

	/// Write file to the input source
	virtual void WriteFile(std::ostream& stream) const;

public:
	L3DFile();

	virtual ~L3DFile() = default;

	/// Read l3d file from the filesystem
	void Open(const std::string& file);

	/// Read l3d file from a buffer
	void Open(const std::vector<uint8_t>& buffer);

	/// Write l3d file to path on the filesystem
	void Write(const std::string& file);

	[[nodiscard]] const std::string& GetFilename() const { return _filename; }
	[[nodiscard]] const L3DHeader& GetHeader() const { return _header; }
	[[nodiscard]] const std::vector<L3DSubmeshHeader>& GetSubmeshHeaders() const { return _submeshHeaders; }
	[[nodiscard]] const std::vector<L3DTexture>& GetSkins() const { return _skins; }
	[[nodiscard]] const std::vector<L3DPoint>& GetExtraPoints() const { return _extraPoints; }
	[[nodiscard]] const std::vector<L3DPrimitiveHeader>& GetPrimitiveHeaders() const { return _primitiveHeaders; }
	[[nodiscard]] const std::vector<L3DVertex>& GetVertices() const { return _vertices; }
	[[nodiscard]] const std::vector<uint16_t>& GetIndices() const { return _indices; }
	[[nodiscard]] const std::vector<L3DVertexGroup>& GetLookUpTableData() const { return _vertexGroups; }
	[[nodiscard]] const std::vector<L3DBlend>& GetBlends() const { return _blends; }
	[[nodiscard]] const std::vector<L3DBone>& GetBones() const { return _bones; }
	[[nodiscard]] const std::span<L3DPrimitiveHeader>& GetPrimitiveSpan(uint32_t submeshIndex) const
	{
		return _primitiveSpans[submeshIndex];
	}
	[[nodiscard]] const std::span<L3DBone>& GetBoneSpan(uint32_t submeshIndex) const { return _boneSpans[submeshIndex]; }
	[[nodiscard]] const std::span<L3DVertex>& GetVertexSpan(uint32_t submeshIndex) const { return _vertexSpans[submeshIndex]; }
	[[nodiscard]] const std::span<uint16_t>& GetIndexSpan(uint32_t submeshIndex) const { return _indexSpans[submeshIndex]; }
	[[nodiscard]] const std::span<L3DVertexGroup>& GetVertexGroupSpan(uint32_t submeshIndex) const
	{
		return _vertexGroupSpans[submeshIndex];
	}

	void AddSubmesh(const L3DSubmeshHeader& header);
	void AddPrimitives(const std::vector<L3DPrimitiveHeader>& headers);
	void AddVertices(const std::vector<L3DVertex>& vertices);
	void AddIndices(const std::vector<uint16_t>& indices);
	void AddBones(const std::vector<L3DBone>& bones);
};

} // namespace openblack::l3d
