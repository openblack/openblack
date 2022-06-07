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
#include <filesystem>
#include <string>
#include <vector>

namespace openblack::lnd
{

struct LNDHeader
{
	uint32_t blockCount;
	std::array<uint8_t, 0x400> lookUpTable;
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
	std::array<uint8_t, 0x10> unknown; // TODO(#455): decode what these are
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
		uint8_t fullWater : 1; // TODO(#456): could mean deep water
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
	std::array<LNDCell, 17 * 17> cells;
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
	std::array<std::array<float, 3>, 4> transformUVBefore;
	std::array<std::array<float, 3>, 4> transformUVAfter;
	uint32_t nextSortingPtr;
	float valueSorting;
	float lowResTexture;
	float fuLrs; ///< (iuLrs / 256)
	float fvLrs; ///< (ivLrs / 256)
	float iuLrs; ///< lowrestex x
	float ivLrs; ///< lowrestex z
	uint32_t smallTextUpdated;
};
static_assert(sizeof(LNDBlock) == 2520);

struct LNDMapMaterial
{
	std::array<uint32_t, 2> indices; ///< Indices of both Materials
	uint32_t coefficient;
};
static_assert(sizeof(LNDMapMaterial) == 3 * sizeof(uint32_t));

struct LNDCountry
{
	uint32_t type;                               ///< Type of terrain
	std::array<LNDMapMaterial, 0x100> materials; ///< altitude 0-255
};
static_assert(sizeof(LNDCountry) == 3076);

struct LNDMaterial
{
	struct R5G5B5A1
	{
		uint16_t b : 5;
		uint16_t g : 5;
		uint16_t r : 5;
		uint16_t a : 1;
	};
	static_assert(sizeof(R5G5B5A1) == sizeof(uint16_t));
	static constexpr uint16_t k_Width = 256;
	static constexpr uint16_t k_Height = 256;

	uint16_t type; ///< Terrain Type
	std::array<R5G5B5A1, k_Width * k_Height> texels;
};
static_assert(sizeof(LNDMaterial) == 0x20002);

struct LNDBumpMap
{
	static constexpr uint16_t k_Width = 256;
	static constexpr uint16_t k_Height = 256;

	std::array<uint8_t, k_Width * k_Height> texels; ///< R8
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
	bool _isLoaded {false};

	std::filesystem::path _filename;

	LNDHeader _header {};
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
	virtual ~LNDFile();

	/// Read l3d file from the filesystem
	void Open(const std::filesystem::path& filepath);

	/// Write l3d file to path on the filesystem
	void Write(const std::filesystem::path& filepath);

	[[nodiscard]] std::string GetFilename() const { return _filename.string(); }
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
