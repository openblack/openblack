/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <filesystem>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace openblack::l3d
{

enum class L3DResult : uint8_t
{
	Success = 0,
	ErrCantOpen,
	ErrFileTooSmall,
	ErrBadHeader,
	ErrBadSubmeshOffset,
	ErrBadSubmeshHeaderOffset,
	ErrBadSkinOffset,
	ErrBadSkinTextureOffset,
	ErrBadPointsOffset,
	ErrBadPrimitiveCount,
	ErrBadPrimitiveOffset,
	ErrBadPrimitiveHeaderOffset,
	ErrBadVertexOffset,
	ErrBadVertexCount,
	ErrBadTriangleOffset,
	ErrBadTriangleCount,
	ErrBadVertexGroupOffset,
	ErrBadVertexGroupCount,
	ErrBadBlendOffset,
	ErrBadBlendCount,
	ErrBadBoneOffset,
	ErrBadBoneCount,
	ErrBadFootprintOffset,
	ErrBadFootprintMeshOffset,
	ErrBadFootprintTextureOffset,
	ErrBadFootprintPixelOffset,
};

std::string_view ResultToStr(L3DResult result);

enum class L3DMeshFlags : uint32_t
{
	None,
	Unknown1 = 1U << 0U,                  // 0x1      (31)
	Unknown2 = 1U << 1U,                  // 0x2      (30)
	Unknown3 = 1U << 2U,                  // 0x4      (29)
	Unknown4 = 1U << 3U,                  // 0x8      (28)
	Unknown5 = 1U << 4U,                  // 0x10     (27)
	Unknown6 = 1U << 5U,                  // 0x20     (26)
	Unknown7 = 1U << 6U,                  // 0x40     (25)
	Unknown8 = 1U << 7U,                  // 0x80     (24)
	HasBones = 1U << 8U,                  // 0x100    (23)
	Unknown10 = 1U << 9U,                 // 0x200    (22) the object is depth-sorted whole
	HasChimney = 1U << 10U,               // 0x400    (21) extra point [1] is where its chimney smokes
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
#pragma pack(push, 1)
	struct alignas(4) Flags
	{
		uint32_t hasBones : 1;
		uint32_t unknown1 : 3;
		uint32_t status : 6;   // Used for scafolds and tomb stones in grave yard
		uint32_t unknown2 : 2; // always 0b10
		uint32_t isWindow : 1;
		uint32_t isPhysics : 1;
		uint32_t unknown3 : 15;
		uint32_t lodMask : 3;
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
		uint16_t b : 4;
		uint16_t g : 4;
		uint16_t r : 4;
		uint16_t a : 4;
	};
	static_assert(sizeof(RGBA4) == sizeof(uint16_t));
	uint32_t id;
	static constexpr uint16_t k_Width = 256;
	static constexpr uint16_t k_Height = 256;
	std::array<RGBA4, k_Width * k_Height> texels;
	static_assert(sizeof(texels) == k_Width * k_Height * sizeof(uint16_t));
};
static_assert(sizeof(L3DTexture) == sizeof(uint32_t) + 256 * 256 * sizeof(uint16_t));

struct L3DBone
{
	uint32_t parent;
	uint32_t firstChild;
	uint32_t rightSibling;
	std::array<float, 9> orientation;
	L3DPoint position;
};
static_assert(sizeof(L3DBone) == 3 * sizeof(uint32_t) + 9 * sizeof(float) + sizeof(L3DPoint));

struct L3DFootprintTriangle
{
	std::array<L3DPoint2D, 3> world;
	std::array<L3DPoint2D, 3> texture;
};

struct L3DFootprintHeader
{
	uint32_t count;
	uint32_t offset;
	uint32_t size;
	uint32_t width;
	uint32_t height;
	uint32_t unknown;
};

struct L3DFootprintEntry
{
	uint32_t unknown1;
	uint32_t unknown2;
	uint32_t triangleCount;
	std::vector<L3DFootprintTriangle> triangles;
	std::vector<uint16_t> pixels; //< ARGB
	uint32_t unknown3;
	uint32_t unknown4;
	uint32_t unknown5;
};

struct L3DFootprintFooter
{
	uint32_t unknown1;
	float unknown2;
	uint32_t unknown3;
};

struct L3DFootprint
{
	L3DFootprintHeader header;
	std::vector<L3DFootprintEntry> entries;
	L3DFootprintFooter footer;
};

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

/// A submesh's lightmap, from the UV2 block. The game multiplies its primitives by the skin, mapped by
/// the vertices' second texture coordinates.
struct L3DLightmap
{
	/// Textured, with the lightmap's skin
	L3DMaterial material;
	std::array<uint32_t, 4> reserved;
};
static_assert(sizeof(L3DLightmap) == 8 * sizeof(uint32_t));

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

/// A vertex of a primitive moved part of the way to another of the same primitive, after each has been placed by its
/// own bone: the seams where the body's parts meet, which would otherwise come apart as the joints bend
struct L3DBlend
{
	/// The vertex that moves, then the vertex it moves towards, counted in the primitive
	std::array<uint16_t, 2> indices;
	/// How far it moves, at most a half
	float weight;
};
static_assert(sizeof(L3DBlend) == 8);

/// A submesh's record in the name block, one for each submesh in turn
struct L3DSubmeshName
{
	enum Flags : uint32_t
	{
		/// The submesh, a window, sheds a volume of light: the game draws its edges drawn out away from
		/// volumeLightSource, fading as they go
		VolumeLight = 1u << 0,
	};

	std::array<char, 64> name;
	uint32_t flags;
	float unknown1;
	L3DPoint volumeLightSource;
	/// How far the edges are drawn out, before the game scales it
	float volumeLightLength;
	/// The point the submesh turns about, when it has a joint
	L3DPoint jointPivot;
	/// The matrix of the game's table of joints which the submesh is turned by, when it is from 0 to 255. The leaves of
	/// the temple's doors have joints.
	int32_t jointIndex;
	/// The submesh's frame, which the temple's rooms place their controls' text and camera by: its x, y and z axes,
	/// then its origin. A point in the frame is x * axes[0] + y * axes[1] + z * axes[2] + origin in the mesh.
	std::array<L3DPoint, 3> frameAxes;
	L3DPoint frameOrigin;
	std::array<float, 12> unknown2;
	/// The submesh's box, in its frame
	L3DPoint frameMin;
	L3DPoint frameMax;
};
static_assert(sizeof(L3DSubmeshName) == 0xE0);

/// Decodes the records of a name block whose data, after its size, count and offset of the records, are at dataOffset
/// in its file. False when the records don't lie in the data.
bool DecodeSubmeshNames(std::span<const uint8_t> data, uint32_t dataOffset, uint32_t count, uint32_t recordsOffset,
                        std::vector<L3DSubmeshName>& names) noexcept;

/// Decodes the lightmaps of a UV2 block at blockOffset in its file, blockSize bytes long with its header: data are its
/// bytes after the block's size, vertex count and submesh count. False when the block doesn't hold them.
bool DecodeLightmaps(std::span<const uint8_t> data, uint32_t blockOffset, uint32_t blockSize, uint32_t vertexCount,
                     uint32_t submeshCount, std::vector<L3DPoint2D>& coordinates, std::vector<L3DLightmap>& lightmaps) noexcept;

/// EBone block (ContainsEBone), after the extra metrics: up to 16 points attached to bones. The game's animal ground
/// blobs use the positions of the first 2 or 4, in the space of the bone they name (-1 = unused).
struct L3DEBone
{
	uint32_t size;                                     ///< 836
	std::array<std::array<float, 3 * 4>, 16> matrices; ///< 3x3 rotation then position
	std::array<int32_t, 16> bones;
};
static_assert(sizeof(L3DEBone) == 836);

/**
  This class is used to read L3Ds.
 */
class L3DFile
{
protected:
	static constexpr const std::array<char, 4> k_Magic = {'L', '3', 'D', '0'};

	/// True when a file has been loaded
	bool _isLoaded {false};

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
	/// Each submesh's blends, its primitives' in turn
	std::vector<std::span<L3DBlend>> _blendSpans;
	std::vector<std::span<L3DBone>> _boneSpans;
	std::optional<L3DFootprint> _footprint;
	std::vector<uint8_t> _uv2Data;
	/// The UV2 block's lightmap coordinates, one for each vertex of every submesh in turn
	std::vector<L3DPoint2D> _lightmapCoordinates;
	/// The UV2 block's lightmap of each submesh
	std::vector<L3DLightmap> _lightmaps;
	std::string _nameData;
	/// The name block's record of each submesh
	std::vector<L3DSubmeshName> _submeshNames;
	std::vector<std::array<float, 3 * 4>> _extraMetrics;
	std::optional<L3DEBone> _eBone;

	/// Write file to the input source
	L3DResult WriteFile(std::ostream& stream) const noexcept;

public:
	L3DFile() noexcept;
	virtual ~L3DFile() noexcept;
	/// A copy's spans of each submesh's data are into its own data
	L3DFile(const L3DFile& other);
	L3DFile& operator=(const L3DFile& other);
	L3DFile(L3DFile&& other) noexcept = default;
	L3DFile& operator=(L3DFile&& other) noexcept = default;

	/// Read file from the input source
	L3DResult ReadFile(std::istream& stream) noexcept;

	/// Read l3d file from the filesystem
	L3DResult Open(const std::filesystem::path& filepath) noexcept;

	/// Read l3d file from a buffer
	L3DResult Open(const std::vector<uint8_t>& buffer) noexcept;

	/// Write l3d file to path on the filesystem
	L3DResult Write(const std::filesystem::path& filepath) noexcept;

	[[nodiscard]] const L3DHeader& GetHeader() const noexcept { return _header; }
	[[nodiscard]] const std::vector<L3DSubmeshHeader>& GetSubmeshHeaders() const noexcept { return _submeshHeaders; }
	[[nodiscard]] const std::vector<L3DTexture>& GetSkins() const noexcept { return _skins; }
	[[nodiscard]] const std::vector<L3DPoint>& GetExtraPoints() const noexcept { return _extraPoints; }
	[[nodiscard]] const std::vector<L3DPrimitiveHeader>& GetPrimitiveHeaders() const noexcept { return _primitiveHeaders; }
	[[nodiscard]] const std::vector<L3DVertex>& GetVertices() const noexcept { return _vertices; }
	/// The vertices to change in place, all of the submeshes' in turn
	[[nodiscard]] std::span<L3DVertex> EditVertices() noexcept { return _vertices; }
	/// The skins to change in place
	[[nodiscard]] std::span<L3DTexture> EditSkins() noexcept { return _skins; }
	[[nodiscard]] const std::vector<uint16_t>& GetIndices() const noexcept { return _indices; }
	[[nodiscard]] const std::vector<L3DVertexGroup>& GetLookUpTableData() const noexcept { return _vertexGroups; }
	[[nodiscard]] const std::vector<L3DBlend>& GetBlends() const noexcept { return _blends; }
	[[nodiscard]] const std::vector<L3DBone>& GetBones() const noexcept { return _bones; }
	[[nodiscard]] const std::optional<L3DFootprint>& GetFootprint() const noexcept { return _footprint; }
	[[nodiscard]] const std::vector<std::array<float, 3 * 4>>& GetExtraMetrics() const noexcept { return _extraMetrics; }
	[[nodiscard]] const std::vector<uint8_t>& GetUv2Data() const noexcept { return _uv2Data; }
	[[nodiscard]] const std::vector<L3DPoint2D>& GetLightmapCoordinates() const noexcept { return _lightmapCoordinates; }
	[[nodiscard]] const std::vector<L3DLightmap>& GetLightmaps() const noexcept { return _lightmaps; }
	[[nodiscard]] const std::optional<L3DEBone>& GetEBone() const noexcept { return _eBone; }
	void SetFootprint(const L3DFootprint& footprint) noexcept { _footprint = footprint; }
	void SetExtraMetrics(const std::vector<std::array<float, 3 * 4>>& metrics) noexcept { _extraMetrics = metrics; }
	void SetUv2Data(std::vector<uint8_t>& uv2Data) noexcept { _uv2Data = uv2Data; }
	void SetNameData(std::string& nameData) noexcept { _nameData = nameData; }
	[[nodiscard]] const std::string& GetNameData() const noexcept { return _nameData; }
	[[nodiscard]] const std::vector<L3DSubmeshName>& GetSubmeshNames() const noexcept { return _submeshNames; }
	[[nodiscard]] const std::span<L3DPrimitiveHeader>& GetPrimitiveSpan(uint32_t submeshIndex) const noexcept
	{
		return _primitiveSpans[submeshIndex];
	}
	[[nodiscard]] const std::span<L3DBone>& GetBoneSpan(uint32_t submeshIndex) const noexcept
	{
		return _boneSpans[submeshIndex];
	}
	[[nodiscard]] const std::span<L3DVertex>& GetVertexSpan(uint32_t submeshIndex) const noexcept
	{
		return _vertexSpans[submeshIndex];
	}
	[[nodiscard]] const std::span<uint16_t>& GetIndexSpan(uint32_t submeshIndex) const noexcept
	{
		return _indexSpans[submeshIndex];
	}
	[[nodiscard]] const std::span<L3DVertexGroup>& GetVertexGroupSpan(uint32_t submeshIndex) const noexcept
	{
		return _vertexGroupSpans[submeshIndex];
	}
	/// A submesh's blends, each primitive's in turn, as many as each primitive's header counts
	[[nodiscard]] std::span<const L3DBlend> GetBlendSpan(uint32_t submeshIndex) const noexcept
	{
		return submeshIndex < _blendSpans.size() ? std::span<const L3DBlend>(_blendSpans[submeshIndex])
		                                         : std::span<const L3DBlend> {};
	}

	void AddSubmesh(const L3DSubmeshHeader& header) noexcept;
	void AddPrimitives(const std::vector<L3DPrimitiveHeader>& headers) noexcept;
	void AddVertices(const std::vector<L3DVertex>& vertices) noexcept;
	void AddIndices(const std::vector<uint16_t>& indices) noexcept;
	void AddBones(const std::vector<L3DBone>& bones) noexcept;
};

} // namespace openblack::l3d
