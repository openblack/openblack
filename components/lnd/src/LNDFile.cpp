/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

/*
 *
 * The layout of a LND File is as follows:
 *
 * - 1052 byte header, containing ints and a 1024 table
 *         block count - number of blocks in block section (see below)
 *         look-up table - block index look-up table
 *         material count - number of materials in materials section (see below)
 *         country count - number of countries in country section (see below)
 *         block size - size of each block in blocks section (see below)
 *         material size - size of each material in materials section (see
 *                         below)
 *         country size - size of each country in country section (see below)
 *         low resolution count - number of low-resolution textures (see below)
 *
 * ------------------------ start of low resolution textures block -------------
 *
 * - variable size * low resolution count, each record contains:
 *         unknown 16 bytes - TODO(bwrsandman)
 *         texture size - Size of the S3 data plus the variable itself
 *         S3 texture data - Texture compressed using S3
 *
 * ------------------------ start of land block block --------------------------
 *
 * - block size (2520 bytes) * block count, each record contains:
 *         a 17 by 17 grid of cells, each cell contains:
 *                 r, g, b values of 1 byte each
 *                 1 byte luminosity value
 *                 1 byte altitude value
 *                 1 byte save color boolean
 *                 1 byte flag for properties where:
 *                         The upper 4 bits represent the country code
 *                         1 bit represents if there is water
 *                         1 bit represents if it is a coast line
 *                         1 bit represents if it is fully water TODO: Deep Water?
 *                         1 bit represents if there is a split
 *                 1 byte flag for sound effects
 *         index
 *         map X
 *         map Z
 *         block X
 *         block Z
 *         clipped
 *         frameVisibility
 *         highestAltitude
 *         useSmallBump
 *         forceLowResTex
 *         meshLOD
 *         meshBlending
 *         textureBlend
 *         meshLODType
 *         fog
 *         texPointer
 *         matPointer
 *         drawSomething
 *         specMatBeforePtr
 *         specMatAfterPtr
 *         transformUVBefore - a matrix
 *         transformUVAfter - a matrix
 *         nextSortingPtr
 *         valueSorting
 *         lowResTexture
 *         fu_lrs
 *         fv_lrs
 *         iu_lrs
 *         iv_lrs
 *         smallTextUpdated
 *
 * ------------------------ start of county block ------------------------------
 *
 * - country size (3076 bytes) * country count, each record contains:
 *         type - type of terrain
 *         sized 256 map, each record contains:
 *                 two material indices
 *                 coefficient
 *
 * ------------------------ start of material block ----------------------------
 *
 * - material size (131074 bytes) * material count, each record contains:
 *         type - type of terrain
 *         texture data - size of 256 by 256 of R5G5B5A1 encoded texels
 *
 * ------------------------ start of extra texture block -----------------------
 *
 * - 2 * 65536, each texture contains:
 *         noise - noise texture
 *         bump - bump map
 *
 * ------------------------ start of unaccounted block -------------------------
 *
 * // TODO(bwrsandman)
 *
 * ------------------------ end of file ----------------------------------------
 */

#include "LNDFile.h"

#include <cassert>
#include <cstring>

#include <fstream>
#include <stdexcept>
#include <utility>

using namespace openblack::lnd;

LNDFile::LNDFile() noexcept = default;
LNDFile::~LNDFile() noexcept = default;

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
		return seekoff(pos - pos_type(static_cast<off_type>(0)), std::ios_base::beg, which);
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

std::string_view openblack::lnd::ResultToStr(LNDResult result)
{
	switch (result)
	{
	case LNDResult::Success:
		return "Success";
	case LNDResult::ErrCantOpen:
		return "Could not open file.";
	case LNDResult::ErrFileTooSmall:
		return "File too small to be a valid LND file.";
	case LNDResult::ErrNonStandardBlockSize:
		return "File has non standard block size.";
	case LNDResult::ErrNonStandardMaterialSize:
		return "File has non standard material size.";
	case LNDResult::ErrNonStandardCountrySize:
		return "File has non standard country size.";
	case LNDResult::ErrBadBlockSize:
		return "Blocks are beyond the end of the file.";
	case LNDResult::ErrBadMaterialSize:
		return "Materials are beyond the end of the file.";
	case LNDResult::ErrBadCountrySize:
		return "Countries are beyond the end of the file.";
	case LNDResult::ErrExtraTextureData:
		return "Extra Textures are beyond the end of the file.";
	case LNDResult::ErrUnaccountedData:
		return "Parsing ended without reaching end of file.";
	}
	std::unreachable();
}

LNDResult LNDFile::ReadFile(std::istream& stream) noexcept
{
	assert(!_isLoaded);

	// Total file size
	std::size_t fsize = 0;
	if (stream.seekg(0, std::ios_base::end))
	{
		fsize = static_cast<std::size_t>(stream.tellg());
		stream.seekg(0);
	}

	if (fsize < sizeof(LNDHeader))
	{
		return LNDResult::ErrFileTooSmall;
	}

	// First 1052 bytes
	stream.read(reinterpret_cast<char*>(&_header), sizeof(LNDHeader));

	if (_header.blockSize != sizeof(LNDBlock))
	{
		return LNDResult::ErrNonStandardBlockSize;
	}
	if (_header.materialSize != sizeof(LNDMaterial))
	{
		return LNDResult::ErrNonStandardMaterialSize;
	}
	if (_header.countrySize != sizeof(LNDCountry))
	{
		return LNDResult::ErrNonStandardCountrySize;
	}

	// Read low resolution textures
	_lowResolutionTextures.resize(_header.lowResolutionCount);
	for (auto& texture : _lowResolutionTextures)
	{
		stream.read(reinterpret_cast<char*>(&texture.header), sizeof(texture.header));
		texture.texels.resize(texture.header.size - sizeof(texture.header.size));
		stream.read(reinterpret_cast<char*>(texture.texels.data()), texture.texels.size() * sizeof(texture.texels[0]));
	}

	// Read Blocks
	// take away a block from the count, because it's not in the file?
	_blocks.resize(_header.blockCount - 1);
	if (static_cast<std::size_t>(stream.tellg()) + _blocks.size() * sizeof(_blocks[0]) > fsize)
	{
		return LNDResult::ErrBadBlockSize;
	}
	stream.read(reinterpret_cast<char*>(_blocks.data()), _blocks.size() * sizeof(_blocks[0]));

	// Read Countries
	_countries.resize(_header.countryCount);
	if (static_cast<std::size_t>(stream.tellg()) + _countries.size() * sizeof(_countries[0]) > fsize)
	{
		return LNDResult::ErrBadCountrySize;
	}
	stream.read(reinterpret_cast<char*>(_countries.data()), _countries.size() * sizeof(_countries[0]));

	// Read Materials
	_materials.resize(_header.materialCount);
	if (static_cast<std::size_t>(stream.tellg()) + _materials.size() * sizeof(_materials[0]) > fsize)
	{
		return LNDResult::ErrBadMaterialSize;
	}
	stream.read(reinterpret_cast<char*>(_materials.data()), _materials.size() * sizeof(_materials[0]));

	// Read Extra textures (noise and bump map)
	if (static_cast<std::size_t>(stream.tellg()) + sizeof(_extra) > fsize)
	{
		return LNDResult::ErrExtraTextureData;
	}
	stream.read(reinterpret_cast<char*>(&_extra), sizeof(_extra));

	// Get all bytes that weren't read
	if (stream.peek() != EOF)
	{
		_unaccounted.resize(static_cast<std::size_t>(fsize - stream.tellg()));
		stream.read(reinterpret_cast<char*>(_unaccounted.data()), _unaccounted.size() * sizeof(_unaccounted[0]));
	}

	if (stream.peek() != EOF)
	{
		return LNDResult::ErrUnaccountedData;
	}

	return LNDResult::Success;
}

LNDResult LNDFile::WriteFile(std::ostream& stream) const noexcept
{
	// First 1052 bytes
	stream.write(reinterpret_cast<const char*>(&_header), sizeof(LNDHeader));

	// Write low resolution textures
	for (const auto& texture : _lowResolutionTextures)
	{
		stream.write(reinterpret_cast<const char*>(&texture.header), sizeof(texture.header));
		stream.write(reinterpret_cast<const char*>(texture.texels.data()), texture.texels.size() * sizeof(texture.texels[0]));
	}

	// Write Blocks
	stream.write(reinterpret_cast<const char*>(_blocks.data()), _blocks.size() * sizeof(_blocks[0]));

	// Write Countries
	stream.write(reinterpret_cast<const char*>(_countries.data()), _countries.size() * sizeof(_countries[0]));

	// Write Materials
	stream.write(reinterpret_cast<const char*>(_materials.data()), _materials.size() * sizeof(_materials[0]));

	// Write Extra textures (noise and bump map)
	stream.write(reinterpret_cast<const char*>(&_extra), sizeof(_extra));

	// TODO(bwrsandman): Figure out what the unaccounted bytes are for and write
	//                   them to the file
	stream.write(reinterpret_cast<const char*>(_unaccounted.data()), _unaccounted.size() * sizeof(_unaccounted[0]));

	return LNDResult::Success;
}

LNDResult LNDFile::Open(const std::filesystem::path& filepath) noexcept
{
	assert(!_isLoaded);

	std::ifstream stream(filepath, std::ios::binary);

	if (!stream.is_open())
	{
		return LNDResult::ErrCantOpen;
	}

	return ReadFile(stream);
}

LNDResult LNDFile::Open(const std::vector<uint8_t>& buffer) noexcept
{
	assert(!_isLoaded);

	imemstream stream(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(buffer[0]));

	return ReadFile(stream);
}

LNDResult LNDFile::Write(const std::filesystem::path& filepath) noexcept
{
	assert(!_isLoaded);

	std::ofstream stream(filepath, std::ios::binary);

	if (!stream.is_open())
	{
		return LNDResult::ErrCantOpen;
	}

	// Prepare header
	_header.blockCount = static_cast<uint32_t>(_blocks.size() + 1);
	memset(_header.lookUpTable.data(), 1, _header.lookUpTable.size());
	_header.materialCount = static_cast<uint32_t>(_materials.size());
	_header.countryCount = static_cast<uint32_t>(_countries.size());
	_header.blockSize = static_cast<uint32_t>(sizeof(LNDBlock));
	_header.materialSize = static_cast<uint32_t>(sizeof(LNDMaterial));
	_header.countrySize = static_cast<uint32_t>(sizeof(LNDCountry));
	_header.lowResolutionCount = static_cast<uint32_t>(_lowResolutionTextures.size());
	for (const auto& block : _blocks)
	{
		const auto lookupIndex = block.blockX << 5 | block.blockZ;
		_header.lookUpTable.at(lookupIndex) = static_cast<uint8_t>(block.index);
	}

	// TODO (#749) use std::views::enumerate
	for (int i = 1; auto& b : _blocks)
	{
		b.index = i;
		++i;
	}

	// TODO (#749) use std::views::enumerate
	for (int i = 0; auto& t : _lowResolutionTextures)
	{
		t.header.index = i;
		t.header.size = static_cast<uint32_t>(t.texels.size()) + 4;
		++i;
	}

	return WriteFile(stream);
}

void LNDFile::AddLowResolutionTexture(const LNDLowResolutionTexture& texture) noexcept
{
	_lowResolutionTextures.emplace_back(texture);
}

void LNDFile::AddMaterial(const LNDMaterial& material) noexcept
{
	_materials.emplace_back(material);
}

void LNDFile::AddNoiseMap(const LNDBumpMap& noiseMap) noexcept
{
	_extra.noise = noiseMap;
}

void LNDFile::AddBumpMap(const LNDBumpMap& bumpMap) noexcept
{
	_extra.bump = bumpMap;
}

void LNDFile::AddBlock(const LNDBlock& block) noexcept
{
	_blocks.emplace_back(block).index = static_cast<uint32_t>(_blocks.size()) + 1;
}

void LNDFile::AddCountry(const LNDCountry& country) noexcept
{
	_countries.emplace_back(country);
}
