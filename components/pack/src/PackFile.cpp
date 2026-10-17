/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

/*
 *
 * The layout of a Pack File is as follows:
 *
 * - 8 byte magic header, containing the chars "LiOnHeAd"
 * - A number of blocks, one having the name MESHES, INFO,
 *   LHAudioBankSampleTable and the rest containing textures. The blocks are
 *   concatenated one after the other. (see below)
 *
 * ------------------------- start of block ------------------------------------
 *
 * - 36 byte header containing:
 *         32 char name
 *         4 byte size of the block
 * - arbitrary size body based on size in head
 *
 * ------------------------- end of file ---------------------------------------
 *
 * The layout of a MESHES block is as follows:
 *
 * - 4 byte magic header, containing the chars "MKJC"
 * - 4 byte int, containing the number of L3D meshes contained. The meshes are
 *   concatenated one after the other within the block (see below).
 *
 * -----------------------------------------------------------------------------
 *
 * The layout of a Body block is as follows:
 *
 * - 4 byte magic header, containing the chars "MKJC"
 * - 4 byte int, containing the number of ANM animations contained. The
 *   animation matadata are concatenated one after the other within the block
 *   (see below).
 *
 * ------------------------- start of animation metadata -----------------------
 *
 * - 4 bytes offset into the block
 * - 4 bytes of unknown data
 *
 * ------------------------- end of Body block ---------------------------------
 *
 * - 4 bytes offset into the block
 * - arbitrary size of body based on the size of an L3DMesh
 *
 * ------------------------- end of MESHES block -------------------------------
 *
 * The layout of a INFO block is as follows:
 *
 * - 4 byte int, containing the number of textures in the block.
 * - 8 byte look-up table * number of textures, containing
 *         block id - integer whose hexadecimal string corresponds of a block in
 *                    the file.
 *         unknown - TODO: Maybe type? Maybe layers?
 *
 * ------------------------- end of INFO block ---------------------------------
 *
 * The layout of a LHAudioBankSampleTable block is as follows:
 *
 * - 2 byte int, containing the number of sound samples in the block.
 * - 2 byte int, unknown
 * - 640 byte audio metadata * number of sound samples, containing
 *         name - 256 characters
 *         unknown - TODO: 4 bytes
 *         id - 4 bytes
 *         if the sample is a bank - 4 bytes
 *         the size of the sample - 4 bytes
 *         the offset of the sample - 4 bytes
 *         if the sample is a clone - 4 bytes
 *         group - 2 bytes
 *         atmospheric group - 2 bytes
 *         4 unknowns - TODO: 4 bytes, 4 bytes, 2 bytes, 2 bytes
 *         sample rate - 4 bytes
 *         5 unknowns - TODO: 2 bytes, 2 bytes, 2 bytes, 2 bytes, 4 bytes
 *         start - 4 bytes
 *         end - 4 bytes
 *         description - 256 characters
 *         priority - 2 bytes
 *         3 unknowns - TODO: 2 bytes, 2 bytes, 2 bytes
 *         loop - 2 bytes
 *         start - 2 bytes
 *         pan - 1 byte
 *         unknowns - TODO: 2 bytes
 *         position - 3 * 32 bit floats
 *         volume - 1 byte
 *         user parameters - 2 bytes
 *         pitch - 2 bytes
 *         unknown - 2 bytes
 *         pitchDeviation - 2 bytes
 *         unknown - 2 bytes
 *         minimum distance - 32 bit float
 *         maximum distance - 32 bit float
 *         scale - 32 bit float
 *         loop type - 16 bit enumeration where: 0 is None, 1 is Restart, 2 is Once and 3 is Overlap
 *         3 unknowns - TODO: 2 bytes, 2 bytes, 2 bytes
 *         atmosphere - 2 bytes
 *
 * ------------------------- end of LHAudioBankSampleTable block ---------------
 *
 * The layout of a texture block is as follows:
 *
 * - 16 byte header containing 4 ints
 *         size - size of the block
 *         block id - integer whose hexadecimal string corresponds of a block in
 *                    the file.
 *         type - TODO: unknown, maybe it corresponds to the unknown in lookup
 *         dds file size - size of the dds file minus magic number
 * - variable size dds file without the first 4 byte magic number
 *
 * The base game uses DXT1 and DXT3 textures
 * The Creature Isle also uses DXT5 textures
 *
 * ------------------------- end of texture block ------------------------------
 *
 */

#include "PackFile.h"

#include <cassert>
#include <cstring>

#include <fstream>
#include <spanstream>
#include <utility>

using namespace openblack::pack;

namespace
{

inline std::span<const char> ReadSubspan(std::ispanstream& stream, size_t size)
{
	auto result = stream.span().subspan(static_cast<size_t>(stream.tellg()), size);
	stream.seekg(size, std::ios_base::cur);
	return result;
}

inline std::ispanstream GetBlockAsStream(const PackFile& pack, const std::string& name)
{
	const auto& data = pack.GetBlock(name);
	auto span = std::span(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(data[0]));
	return std::ispanstream {span};
}

struct PackBlockHeader
{
	static constexpr uint32_t k_BlockNameSize = 0x20;
	std::array<char, k_BlockNameSize> blockName;
	uint32_t blockSize;
};

/// Magic Key Jean-Claude Cottier
constexpr const std::array<char, 4> k_BlockMagic = {'M', 'K', 'J', 'C'};
} // namespace

std::string_view openblack::pack::ResultToStr(PackResult result)
{
	switch (result)
	{
	case PackResult::Success:
		return "Success";
	case PackResult::ErrCantOpen:
		return "Could not open file.";
	case PackResult::ErrFileTooSmall:
		return "File too small to be a valid Pack file.";
	case PackResult::ErrNoEntries:
		return "There are no sound entries in block.";
	case PackResult::ErrUnrecognizedHeader:
		return "Unrecognized Pack header.";
	case PackResult::ErrUnrecognizedBlockHeader:
		return "Unrecognized Pack Block header.";
	case PackResult::ErrFileNotEvenlySplit:
		return "File not evenly split into whole blocks.";
	case PackResult::ErrDuplicateBlockName:
		return "Duplicate block name.";
	case PackResult::ErrMissingInfoBlock:
		return "No INFO block in pack.";
	case PackResult::ErrMissingBodyBlock:
		return "No Body block in pack.";
	case PackResult::ErrMissingAudioBankSampleTableBlock:
		return "No LHAudioBankSampleTable block in pack.";
	case PackResult::ErrMissingAudioWaveDataBlock:
		return "No LHAudioWaveData block in sad pack.";
	case PackResult::ErrMissingTextureBlock:
		return "Required texture block missing.";
	case PackResult::ErrTextureBlockIdMismatch:
		return "Texture block id is not the same as block id.";
	case PackResult::ErrTextureDuplicate:
		return "Duplicate texture extracted.";
	case PackResult::ErrTextureInvalidDDSHeaderSize:
		return "Invalid DDS header sizes.";
	case PackResult::ErrMissingMeshBlock:
		return "No MESHES block in pack.";
	case PackResult::ErrMeshBlockHeaderMalformed:
		return "Unrecognized Mesh Block header.";
	case PackResult::ErrNotImplemented:
		return "Function is not yet implemented.";
	}
	std::unreachable();
}

PackResult PackFile::ReadBlocks(std::istream& stream) noexcept
{
	assert(!_isLoaded);

	// Total file size
	std::size_t fsize = 0;
	if (stream.seekg(0, std::ios_base::end))
	{
		fsize = static_cast<std::size_t>(stream.tellg());
		stream.seekg(0);
	}

	std::array<char, k_Magic.size()> magic;
	if (fsize < magic.size() + sizeof(PackBlockHeader))
	{
		return PackResult::ErrFileTooSmall;
	}

	// First 8 bytes
	stream.read(magic.data(), magic.size());
	if (std::memcmp(magic.data(), k_Magic.data(), magic.size()) != 0)
	{
		return PackResult::ErrUnrecognizedHeader;
	}

	if (fsize < static_cast<std::size_t>(stream.tellg()) + sizeof(PackBlockHeader))
	{
		return PackResult::ErrFileTooSmall;
	}

	PackBlockHeader header;
	while (fsize - sizeof(PackBlockHeader) > static_cast<std::size_t>(stream.tellg()))
	{
		stream.read(reinterpret_cast<char*>(&header), sizeof(PackBlockHeader));

		if (_blocks.contains(header.blockName.data()))
		{
			return PackResult::ErrDuplicateBlockName;
		}

		_blocks[std::string(header.blockName.data())] = std::vector<uint8_t>(header.blockSize);
		stream.read(reinterpret_cast<char*>(_blocks[header.blockName.data()].data()), header.blockSize);
	}

	if (fsize < static_cast<std::size_t>(stream.tellg()))
	{
		return PackResult::ErrFileNotEvenlySplit;
	}

	return PackResult::Success;
}

PackResult PackFile::ResolveInfoBlock() noexcept
{
	if (!HasBlock("INFO"))
	{
		return PackResult::ErrMissingInfoBlock;
	}

	auto stream = GetBlockAsStream(*this, "INFO");

	uint32_t totalTextures;
	stream.read(reinterpret_cast<char*>(&totalTextures), sizeof(uint32_t));

	// Read lookup
	_infoBlockLookup.resize(totalTextures);
	stream.read(reinterpret_cast<char*>(_infoBlockLookup.data()), _infoBlockLookup.size() * sizeof(_infoBlockLookup[0]));

	return PackResult::Success;
}

PackResult PackFile::ResolveBodyBlock() noexcept
{
	if (!HasBlock("Body"))
	{
		return PackResult::ErrMissingBodyBlock;
	}

	auto stream = GetBlockAsStream(*this, "Body");

	// Greetings Jean-Claude Cottier
	std::array<char, k_BlockMagic.size()> magic;
	stream.read(magic.data(), magic.size());
	if (std::memcmp(magic.data(), k_BlockMagic.data(), magic.size()) != 0)
	{
		return PackResult::ErrUnrecognizedBlockHeader;
	}

	uint32_t totalAnimations;
	stream.read(reinterpret_cast<char*>(&totalAnimations), sizeof(uint32_t));

	// Read lookup offsets
	_bodyBlockLookup.resize(totalAnimations);
	stream.read(reinterpret_cast<char*>(_bodyBlockLookup.data()), _bodyBlockLookup.size() * sizeof(_bodyBlockLookup[0]));

	return PackResult::Success;
}

PackResult PackFile::ResolveAudioBankSampleTableBlock() noexcept
{
	if (!HasBlock("LHAudioBankSampleTable"))
	{
		return PackResult::ErrMissingAudioBankSampleTableBlock;
	}

	auto stream = GetBlockAsStream(*this, "LHAudioBankSampleTable");
	std::size_t fsize = 0;
	if (stream.seekg(0, std::ios_base::end))
	{
		fsize = static_cast<std::size_t>(stream.tellg());
		stream.seekg(0);
	}

	if (fsize < sizeof(uint32_t))
	{
		return PackResult::ErrFileTooSmall;
	}

	uint16_t sampleCount;
	stream.read(reinterpret_cast<char*>(&sampleCount), sizeof(sampleCount));

	uint16_t unknown;
	stream.read(reinterpret_cast<char*>(&unknown), sizeof(unknown));

	if (sampleCount == 0)
	{
		return PackResult::ErrNoEntries;
	}

	_audioSampleHeaders.resize(sampleCount);

	if (fsize != sizeof(uint32_t) + _audioSampleHeaders.size() * sizeof(_audioSampleHeaders[0]))
	{
		return PackResult::ErrFileTooSmall;
	}

	stream.read(reinterpret_cast<char*>(_audioSampleHeaders.data()),
	            _audioSampleHeaders.size() * sizeof(_audioSampleHeaders[0]));

	return PackResult::Success;
}

PackResult PackFile::ExtractTexturesFromBlock() noexcept
{
	G3DTextureHeader header;
	constexpr uint32_t k_BlockNameSize = 0x20;
	std::array<char, k_BlockNameSize> blockName;
	for (const auto& item : _infoBlockLookup)
	{
		// Convert int id to string representation as hexadecimal key
		std::snprintf(blockName.data(), blockName.size(), "%x", item.blockId);

		if (!HasBlock(blockName.data()))
		{
			return PackResult::ErrMissingTextureBlock;
		}

		auto stream = GetBlockAsStream(*this, blockName.data());

		stream.read(reinterpret_cast<char*>(&header), sizeof(header));

		if (header.id != item.blockId)
		{
			return PackResult::ErrTextureBlockIdMismatch;
		}

		if (_textures.contains(blockName.data()))
		{
			return PackResult::ErrTextureDuplicate;
		}

		auto ddsStream = std::ispanstream(ReadSubspan(stream, header.size));

		DdsHeader ddsHeader;
		ddsStream.read(reinterpret_cast<char*>(&ddsHeader), sizeof(DdsHeader));

		// Verify the header to validate the DDS file
		if (ddsHeader.size != sizeof(DdsHeader) || ddsHeader.format.size != sizeof(DdsPixelFormat))
		{
			return PackResult::ErrTextureInvalidDDSHeaderSize;
		}

		// Handle cases where this field is not provided
		// https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide
		// Some Creature Isle DXT5 textures lack this field
		if (ddsHeader.pitchOrLinearSize == 0)
		{
			// The block-size is 8 bytes for DXT1, BC1, and BC4 formats, and 16 bytes for other block-compressed formats
			int blockSize;
			auto format = std::string(ddsHeader.format.fourCC.data(), ddsHeader.format.fourCC.size());
			if (format == "DXT1" || format == "BC1" || format == "BC4")
			{
				blockSize = 8;
			}
			else
			{
				blockSize = 16;
			}

			ddsHeader.pitchOrLinearSize = ((ddsHeader.width + 3) / 4) * ((ddsHeader.height + 3) / 4) * blockSize;
		}

		std::vector<uint8_t> dssTexels(ddsHeader.pitchOrLinearSize);
		ddsStream.read(reinterpret_cast<char*>(dssTexels.data()), dssTexels.size());

		_textures[blockName.data()] = {header, ddsHeader, dssTexels};
	}

	return PackResult::Success;
}

PackResult PackFile::ExtractAnimationsFromBlock() noexcept
{
	auto stream = GetBlockAsStream(*this, "Body");

	// Read lookup
	constexpr uint32_t k_BlockNameSize = 0x20;
	constexpr uint32_t k_AnimationHeaderSize = 0x54;

	std::array<char, k_BlockNameSize> blockName;
	_animations.resize(_bodyBlockLookup.size());
	for (uint32_t i = 0; i < _bodyBlockLookup.size(); ++i)
	{
		snprintf(blockName.data(), k_BlockNameSize, "Julien%d", i);
		if (!HasBlock(blockName.data()))
		{
			return PackResult::ErrMissingTextureBlock;
		}

		auto animationData = GetBlock(blockName.data());
		_animations[i].resize(k_AnimationHeaderSize + animationData.size());

		stream.seekg(_bodyBlockLookup[i].offset);
		stream.read(reinterpret_cast<char*>(_animations[i].data()), k_AnimationHeaderSize);
		memcpy(_animations[i].data() + k_AnimationHeaderSize, animationData.data(), animationData.size());
	}

	return PackResult::Success;
}

PackResult PackFile::ExtractSoundsFromBlock() noexcept
{
	if (!HasBlock("LHAudioWaveData"))
	{
		return PackResult::ErrMissingAudioWaveDataBlock;
	}

	auto stream = GetBlockAsStream(*this, "LHAudioWaveData");
	//	auto isSector = false;
	//	auto isPrevSector = false;

	_audioSampleData.resize(_audioSampleHeaders.size());
	for (int i = 0; const auto& sample : _audioSampleHeaders)
	{
		if (sample.offset > stream.span().size())
		{
			return PackResult::ErrFileTooSmall;
		}
		if (sample.offset + sample.size > stream.span().size())
		{
			return PackResult::ErrFileTooSmall;
		}

		stream.seekg(sample.offset);
		_audioSampleData[i] = ReadSubspan(stream, sample.size);

		++i;
	}

	return PackResult::Success;
}

PackResult PackFile::ResolveMeshBlock() noexcept
{
	if (!HasBlock("MESHES"))
	{
		return PackResult::ErrMissingMeshBlock;
	}

	auto stream = GetBlockAsStream(*this, "MESHES");

	// Greetings Jean-Claude Cottier
	std::array<char, k_BlockMagic.size()> magic;
	stream.read(magic.data(), magic.size());
	if (std::memcmp(magic.data(), k_BlockMagic.data(), magic.size()) != 0)
	{
		return PackResult::ErrMeshBlockHeaderMalformed;
	}

	uint32_t meshCount;
	stream.read(reinterpret_cast<char*>(&meshCount), sizeof(meshCount));
	std::vector<uint32_t> meshOffsets(meshCount);
	stream.read(reinterpret_cast<char*>(meshOffsets.data()), meshOffsets.size() * sizeof(meshOffsets[0]));

	_meshes.resize(meshOffsets.size());
	for (std::size_t i = 0; i < _meshes.size(); i++)
	{
		auto size = (i == _meshes.size() - 1 ? stream.span().size() : meshOffsets[i + 1]) - meshOffsets[i];
		_meshes[i] = ReadSubspan(stream, size);
	}

	return PackResult::Success;
}

PackResult PackFile::WriteBlocks(std::ostream& stream) const noexcept
{
	assert(!_isLoaded);

	// Magic number
	stream.write(k_Magic.data(), k_Magic.size());

	PackBlockHeader header;

	for (const auto& [name, contents] : _blocks)
	{
		std::snprintf(header.blockName.data(), header.blockName.size(), "%s", name.c_str());
		header.blockSize = static_cast<uint32_t>(contents.size() * sizeof(contents[0]));

		stream.write(reinterpret_cast<const char*>(&header), sizeof(header));
		stream.write(reinterpret_cast<const char*>(contents.data()), header.blockSize);
	}

	return PackResult::Success;
}

PackResult PackFile::CreateTextureBlocks() noexcept
{
	// TODO(bwrsandman): Loop through every texture and create a block with
	//                   their id. Then fill in the look-up table.
	assert(_isLoaded);

	return PackResult::ErrNotImplemented;
}

PackResult PackFile::CreateRawBlock(const std::string& name, std::vector<uint8_t>&& data) noexcept
{
	if (HasBlock(name))
	{
		return PackResult::ErrDuplicateBlockName;
	}

	_blocks[name] = std::move(data);

	return PackResult::Success;
}

PackResult PackFile::CreateMeshBlock() noexcept
{
	if (HasBlock("MESHES"))
	{
		return PackResult::ErrDuplicateBlockName;
	}

	size_t offset = 0;
	std::vector<uint8_t> contents;

	auto meshCount = static_cast<uint32_t>(_meshes.size());
	contents.resize(k_BlockMagic.size() + sizeof(meshCount));

	std::memcpy(contents.data() + offset, k_BlockMagic.data(), k_BlockMagic.size());
	offset += k_BlockMagic.size();
	std::memcpy(contents.data() + offset, &meshCount, sizeof(meshCount));
	offset += sizeof(meshCount);

	contents.resize(offset + sizeof(uint32_t) * meshCount);
	if (meshCount > 0)
	{
		const auto previousOffset = offset;
		offset += sizeof(uint32_t) * meshCount;
		for (size_t i = 0; i < _meshes.size(); ++i)
		{
			auto* meshOffsets = reinterpret_cast<uint32_t*>(&contents[previousOffset]);
			meshOffsets[i] = static_cast<uint32_t>(offset);
			offset += _meshes[i].size();
		}
		contents.resize(offset);
		for (size_t i = 0; i < _meshes.size(); ++i)
		{
			if (!_meshes[i].empty())
			{
				auto meshOffset = reinterpret_cast<uint32_t*>(&contents[previousOffset])[i];
				std::memcpy(&contents[meshOffset], _meshes[i].data(), _meshes[i].size() * sizeof(_meshes[i][0]));
			}
		}
	}

	_blocks["MESHES"] = std::move(contents);

	return PackResult::Success;
}

PackResult PackFile::InsertMesh(std::span<const char> span) noexcept
{
	_meshes.emplace_back(std::move(span));

	return PackResult::Success;
}

PackResult PackFile::CreateInfoBlock() noexcept
{
	if (HasBlock("INFO"))
	{
		return PackResult::ErrDuplicateBlockName;
	}

	uint32_t offset = 0;
	std::vector<uint8_t> contents;

	auto totalTextures = static_cast<uint32_t>(_infoBlockLookup.size());
	contents.resize(sizeof(totalTextures) + _infoBlockLookup.size() * sizeof(_infoBlockLookup[0]));

	std::memcpy(contents.data() + offset, &totalTextures, sizeof(totalTextures));
	offset += static_cast<uint32_t>(sizeof(totalTextures));

	std::memcpy(contents.data() + offset, _infoBlockLookup.data(), _infoBlockLookup.size() * sizeof(_infoBlockLookup[0]));

	_blocks["INFO"] = std::move(contents);

	return PackResult::Success;
}

PackResult PackFile::CreateBodyBlock() noexcept
{
	if (HasBlock("Body"))
	{
		return PackResult::ErrDuplicateBlockName;
	}

	std::vector<uint8_t> contents;

	_blocks["Body"] = std::move(contents);

	return PackResult::Success;
}

PackFile::PackFile() noexcept = default;
PackFile::~PackFile() noexcept = default;

PackResult PackFile::ReadFile(std::istream& stream) noexcept
{
	PackResult result;

	result = ReadBlocks(stream);
	if (result != PackResult::Success)
	{
		return result;
	}

	// Mesh pack
	if (HasBlock("INFO"))
	{
		result = ResolveInfoBlock();
		if (result != PackResult::Success)
		{
			return result;
		}

		result = ExtractTexturesFromBlock();
		if (result != PackResult::Success)
		{
			return result;
		}

		result = ResolveMeshBlock();
		if (result != PackResult::Success)
		{
			return result;
		}
	}

	// Anim pack
	if (HasBlock("Body"))
	{
		result = ResolveBodyBlock();
		if (result != PackResult::Success)
		{
			return result;
		}

		result = ExtractAnimationsFromBlock();
		if (result != PackResult::Success)
		{
			return result;
		}
	}

	// Sound pack
	if (HasBlock("LHAudioBankSampleTable"))
	{
		result = ResolveAudioBankSampleTableBlock();
		if (result != PackResult::Success)
		{
			return result;
		}

		// ResolveFileSegmentBankBlock();
		// if (result != PackResult::Success)
		// {
		// 	return result;
		// }
		result = ExtractSoundsFromBlock();
		if (result != PackResult::Success)
		{
			return result;
		}
	}

	_isLoaded = true;

	return PackResult::Success;
}

PackResult PackFile::Open(const std::filesystem::path& filepath) noexcept
{
	std::ifstream stream(filepath, std::ios::binary);

	if (!stream.is_open())
	{
		return PackResult::ErrCantOpen;
	}

	return ReadFile(stream);
}

PackResult PackFile::Open(const std::vector<uint8_t>& buffer) noexcept
{
	assert(!_isLoaded);

	auto span = std::span(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(buffer[0]));
	auto stream = std::ispanstream(span);

	return ReadFile(stream);
}

PackResult PackFile::Write(const std::filesystem::path& filepath) noexcept
{
	assert(!_isLoaded);

	std::ofstream stream(filepath, std::ios::binary);

	if (!stream.is_open())
	{
		return PackResult::ErrCantOpen;
	}

	return WriteBlocks(stream);
}
