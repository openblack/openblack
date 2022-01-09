/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <string>
#include <vector>

namespace openblack::lnd
{

struct LNDHeader
{
	uint32_t blockCount;
	uint8_t lookUpTable[1024];
	uint32_t materialCount;
	uint32_t countryCount;
	uint32_t blockSize;
	uint32_t materialSize;
	uint32_t countrySize;
	uint32_t lowResolutionCount;
};
static_assert(sizeof(LNDHeader) == 1052);

struct LNDLowResolutionTextureHeader
{
	uint8_t unknown[16]; // TODO: decode what these are
	uint32_t size;
};
static_assert(sizeof(LNDLowResolutionTextureHeader) == 20);

struct LNDLowResolutionTexture
{
	LNDLowResolutionTextureHeader header;
	std::vector<uint8_t> texels;
};

struct LNDCell
{
	struct Properties
	{
		uint8_t country : 4;
		uint8_t hasWater : 1;
		uint8_t coastLine : 1;
		uint8_t fullWater : 1; // TODO: could mean deep water
		uint8_t split : 1;
	};

	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t luminosity;
	uint8_t altitude;
	uint8_t saveColor;
	Properties properties;
	/// Sound properties: coastal sound, land sound, sea sound, freshwater sound
	uint8_t flags;
};
static_assert(sizeof(LNDCell::Properties) == 1);
static_assert(sizeof(LNDCell) == 8);

/// TODO(bwrsandman): determine if some of these uints are bools
struct LNDBlock
{
	LNDCell cells[17 * 17];
	uint32_t index;            ///< 2312
	float mapX;                ///< 2320
	float mapZ;                ///< 2316
	uint32_t blockX;           ///< 2328
	uint32_t blockZ;           ///< 2324
	uint32_t clipped;          ///< 2332 0
	uint32_t frameVisibility;  ///< 0
	uint32_t highestAltitude;  ///< 0
	uint32_t useSmallBump;     ///< 0
	uint32_t forceLowResTex;   ///< 0
	uint32_t meshLOD;          ///< 0
	uint32_t meshBlending;     ///< 0
	uint32_t textureBlend;     ///< 0
	uint32_t meshLODType;      ///< 0
	uint32_t fog;              ///< 0
	uint32_t texPointer;       ///< 0 if use_always_low_texture, else new Tex
	uint32_t matPointer;       ///< 0 if use_always_low_texture, else new Mat(Tex)
	uint32_t drawSomething;    ///< 0aaa
	uint32_t specMatBeforePtr; ///< 0
	uint32_t specMatAfterPtr;  ///< 0
	float transformUVBefore[3][4];
	float transformUVAfter[3][4];
	uint32_t nextSortingPtr;
	float valueSorting;
	float lowResTexture;
	float fu_lrs; ///< (iu_lrs / 256)
	float fv_lrs; ///< (iv_lrs / 256)
	float iu_lrs; ///< lowrestex x
	float iv_lrs; ///< lowrestex z
	uint32_t smallTextUpdated;
};
static_assert(sizeof(LNDBlock) == 2520);

struct LNDMapMaterial
{
	uint32_t indices[2]; ///< Indices of both Materials
	uint32_t coefficient;
};
static_assert(sizeof(LNDMapMaterial) == 3 * sizeof(uint32_t));

struct LNDCountry
{
	uint32_t type;                 ///< Type of terrain
	LNDMapMaterial materials[256]; ///< altitude 0-255
};
static_assert(sizeof(LNDCountry) == 3076);

struct LNDMaterial
{
	struct R5G5B5A1
	{
		uint16_t B : 5;
		uint16_t G : 5;
		uint16_t R : 5;
		uint16_t A : 1;
	};
	static_assert(sizeof(R5G5B5A1) == sizeof(uint16_t));
	static constexpr uint16_t width = 256;
	static constexpr uint16_t height = 256;

	uint16_t type; ///< Terrain Type
	R5G5B5A1 texels[width * height];
};
static_assert(sizeof(LNDMaterial) == 0x20002);

struct LNDBumpMap
{
	static constexpr uint16_t width = 256;
	static constexpr uint16_t height = 256;

	uint8_t texels[width * height]; ///< R8
};
static_assert(sizeof(LNDBumpMap) == 0x10000);

struct LNDExtraTextures
{
	LNDBumpMap noise;
	LNDBumpMap bump;
};
static_assert(sizeof(LNDExtraTextures) == 0x20000);

/**
  This class is used to read LNDs.
 */
class LNDFile
{
protected:
	/// True when a file has been loaded
	bool _isLoaded;

	std::string _filename;

	LNDHeader _header;
	std::vector<LNDLowResolutionTexture> _lowResolutionTextures;
	std::vector<LNDBlock> _blocks;
	std::vector<LNDCountry> _countries;
	std::vector<LNDMaterial> _materials;
	LNDExtraTextures _extra;
	// TODO(bwrsandman): There should be no accounted bytes. Reverse those bytes
	//                   and remove this array.
	/// Bytes in file which could serve a purpose but are unaccounted for in
	/// current model.
	std::vector<uint8_t> _unaccounted;

	/// Error handling
	void Fail(const std::string& msg);

	/// Read file from the input source
	virtual void ReadFile(std::istream& stream);

	/// Write file to the input source
	virtual void WriteFile(std::ostream& stream) const;

public:
	LNDFile();

	virtual ~LNDFile() = default;

	/// Read l3d file from the filesystem
	void Open(const std::string& file);

	/// Write l3d file to path on the filesystem
	void Write(const std::string& file);

	[[nodiscard]] const std::string& GetFilename() const { return _filename; }
	[[nodiscard]] const auto& GetHeader() const { return _header; }
	[[nodiscard]] const auto& GetLowResolutionTextures() const { return _lowResolutionTextures; }
	[[nodiscard]] const auto& GetBlocks() const { return _blocks; }
	[[nodiscard]] const auto& GetCountries() const { return _countries; }
	[[nodiscard]] const auto& GetMaterials() const { return _materials; }
	[[nodiscard]] const auto& GetExtra() const { return _extra; }
	[[nodiscard]] const auto& GetUnaccounted() const { return _unaccounted; }

	void AddMaterial(const LNDMaterial& material);
	void AddNoiseMap(const LNDBumpMap& noiseMap);
	void AddBumpMap(const LNDBumpMap& bumpMap);
};

} // namespace openblack::lnd
