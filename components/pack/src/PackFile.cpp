/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

/*
 *
 * The layout of a Pack File is as follows:
 *
 * - 8 byte magic header, containing the chars "LiOnHeAd"
 * - A number of blocks, one having the name MESHES, INFO and the rest
 *   containing textures. The blocks are concatenated one after the other.
 *   (see below)
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
 * - 4 byte int, containing the number of textures in the file.
 * - 8 byte look-up table * number of textures, containing
 *         block id - integer whose hexadecimal string corresponds of a block in
 *                    the file.
 *         unknown - TODO: Maybe type? Maybe layers?
 *
 * ------------------------- end of INFO block ---------------------------------
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
 * ------------------------- end of texture block ------------------------------
 *
 */

#include <PackFile.h>

#include <cassert>
#include <cstring>
#include <fstream>

#ifdef HAS_FILESYSTEM
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif // HAS_FILESYSTEM

using namespace openblack::pack;

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
struct imemstream: virtual membuf, public std::istream
{
	imemstream(char const* base, size_t size)
	    : membuf(base, size)
	    , std::istream(dynamic_cast<std::streambuf*>(this))
	{
	}
};

struct PackBlockHeader
{
	static constexpr uint32_t blockNameSize = 32;
	char blockName[blockNameSize];
	uint32_t blockSize;
};

/// Magic Key Jean-Claude Cottier
constexpr const char kBlockMagic[4] = {'M', 'K', 'J', 'C'};
} // namespace

/// Error handling
void PackFile::Fail(const std::string& msg)
{
	throw std::runtime_error("Pack Error: " + msg + "\nFilename: " + _filename);
}

void PackFile::ReadBlocks()
{
	assert(!_isLoaded);

	std::ifstream input(_filename, std::ios::binary);

	if (!input.is_open())
	{
		Fail("Could not open file.");
	}

	// Total file size
	std::size_t fsize = 0;
	if (input.seekg(0, std::ios_base::end))
	{
		fsize = static_cast<std::size_t>(input.tellg());
		input.seekg(0);
	}

	char magic[sizeof(kMagic)];
	if (fsize < sizeof(magic) + sizeof(PackBlockHeader))
	{
		Fail("File too small to be a valid Pack file.");
	}

	// First 8 bytes
	input.read(reinterpret_cast<char*>(&magic), sizeof(magic));
	if (std::memcmp(&magic, kMagic, sizeof(magic)) != 0)
	{
		Fail("Unrecognized Pack header");
	}

	if (fsize < static_cast<std::size_t>(input.tellg()) + sizeof(PackBlockHeader))
	{
		Fail("File too small to contain any blocks.");
	}

	PackBlockHeader header;
	while (fsize - sizeof(PackBlockHeader) > static_cast<std::size_t>(input.tellg()))
	{
		input.read(reinterpret_cast<char*>(&header), sizeof(PackBlockHeader));

		if (_blocks.count(header.blockName) > 0)
		{
			Fail(std::string("Duplicate block name: ") + header.blockName);
		}

		_blocks[std::string(header.blockName)] = std::vector<uint8_t>(header.blockSize);
		input.read(reinterpret_cast<char*>(_blocks[header.blockName].data()), header.blockSize);
	}

	if (fsize < static_cast<std::size_t>(input.tellg()))
	{
		Fail("File not evenly split into whole blocks.");
	}
}

void PackFile::ResolveInfoBlock()
{
	if (!HasBlock("INFO"))
	{
		Fail("no INFO block in mesh pack");
	}

	auto data = GetBlock("INFO");
	imemstream stream(reinterpret_cast<const char*>(data.data()), data.size());

	uint32_t totalTextures;
	stream.read(reinterpret_cast<char*>(&totalTextures), sizeof(uint32_t));

	// Read lookup
	_infoBlockLookup.resize(totalTextures);
	stream.read(reinterpret_cast<char*>(_infoBlockLookup.data()), _infoBlockLookup.size() * sizeof(_infoBlockLookup[0]));
}

void PackFile::ResolveBodyBlock()
{
	if (!HasBlock("Body"))
	{
		Fail("no Body block in anim pack");
	}

	auto data = GetBlock("Body");
	imemstream stream(reinterpret_cast<const char*>(data.data()), data.size());

	// Greetings Jean-Claude Cottier
	char magic[sizeof(kBlockMagic)];
	stream.read(magic, sizeof(magic));
	if (std::memcmp(&magic, kBlockMagic, sizeof(magic)) != 0)
	{
		Fail("Unrecognized Body Block header");
	}

	uint32_t totalAnimations;
	stream.read(reinterpret_cast<char*>(&totalAnimations), sizeof(uint32_t));

	// Read lookup offsets
	_bodyBlockLookup.resize(totalAnimations);
	stream.read(reinterpret_cast<char*>(_bodyBlockLookup.data()), _bodyBlockLookup.size() * sizeof(_bodyBlockLookup[0]));
}

void PackFile::ExtractTexturesFromBlock()
{
	G3DTextureHeader header;
	constexpr uint32_t blockNameSize = 0x20;
	char blockName[blockNameSize];
	for (const auto& item : _infoBlockLookup)
	{
		// Convert int id to string representation as hexadecimal key
		std::snprintf(blockName, sizeof(blockName), "%x", item.blockId);

		if (!HasBlock(blockName))
		{
			Fail(std::string("Required texture block \"") + blockName + "\" missing.");
		}

		auto stream = GetBlockAsStream(blockName);

		stream->read(reinterpret_cast<char*>(&header), sizeof(header));
		std::vector<uint8_t> dds(header.size);
		stream->read(reinterpret_cast<char*>(dds.data()), dds.size() * sizeof(dds[0]));

		if (header.id != item.blockId)
		{
			Fail("Texture block id is not the same as block id");
		}

		if (_textures.count(blockName) > 0)
		{
			Fail("Duplicate texture extracted");
		}

		imemstream ddsStream(reinterpret_cast<const char*>(dds.data()), dds.size());

		DdsHeader ddsHeader;
		ddsStream.read(reinterpret_cast<char*>(&ddsHeader), sizeof(DdsHeader));

		// TODO(bwrsandman) the extra sizeof(uint32_t) is unaccounted for
		if (header.ddsSize - sizeof(ddsHeader) - sizeof(uint32_t) != ddsHeader.pitchOrLinearSize)
		{
			Fail("Size in header does not match according to DDS signature");
		}

		std::vector<uint8_t> dssTexels(ddsHeader.pitchOrLinearSize);
		ddsStream.read(reinterpret_cast<char*>(dssTexels.data()), dssTexels.size());

		_textures[blockName] = {header, ddsHeader, dssTexels};
	}
}

void PackFile::ExtractAnimationsFromBlock()
{
	auto data = GetBlock("Body");
	imemstream stream(reinterpret_cast<const char*>(data.data()), data.size());

	// Read lookup
	constexpr uint32_t blockNameSize = 0x20;
	constexpr uint32_t animationHeaderSize = 0x54;

	char blockName[blockNameSize];
	_animations.resize(_bodyBlockLookup.size());
	for (uint32_t i = 0; i < _bodyBlockLookup.size(); ++i)
	{
		snprintf(blockName, blockNameSize, "Julien%d", i);
		if (!HasBlock(blockName))
		{
			Fail(std::string("Required texture block \"") + blockName + "\" missing.");
		}

		auto animationData = GetBlock(blockName);
		_animations[i].resize(animationHeaderSize + animationData.size());

		stream.seekg(_bodyBlockLookup[i].offset);
		stream.read(reinterpret_cast<char*>(_animations[i].data()), animationHeaderSize);
		memcpy(_animations[i].data() + animationHeaderSize, animationData.data(), animationData.size());
	}
}

void PackFile::ResolveMeshBlock()
{
	if (!HasBlock("MESHES"))
	{
		Fail("no MESHES block in mesh pack");
	}
	auto data = GetBlock("MESHES");

	imemstream stream(reinterpret_cast<const char*>(data.data()), data.size());
	// Greetings Jean-Claude Cottier
	char magic[sizeof(kBlockMagic)];
	stream.read(magic, sizeof(magic));
	if (std::memcmp(&magic, kBlockMagic, sizeof(magic)) != 0)
	{
		Fail("Unrecognized Mesh Block header");
	}

	uint32_t meshCount;
	stream.read(reinterpret_cast<char*>(&meshCount), sizeof(meshCount));
	std::vector<uint32_t> meshOffsets(meshCount);
	stream.read(reinterpret_cast<char*>(meshOffsets.data()), meshOffsets.size() * sizeof(meshOffsets[0]));

	_meshes.resize(meshOffsets.size());
	for (std::size_t i = 0; i < _meshes.size(); i++)
	{
		auto size = (i == _meshes.size() - 1 ? data.size() : meshOffsets[i + 1]) - meshOffsets[i];
		_meshes[i].resize(size);
		stream.read(reinterpret_cast<char*>(_meshes[i].data()), _meshes[i].size() * sizeof(_meshes[i][0]));
	}
}

void PackFile::WriteBlocks(std::ostream& stream) const
{
	assert(!_isLoaded);

	// Magic number
	stream.write(kMagic, sizeof(kMagic));

	PackBlockHeader header;

	for (auto& [name, contents] : _blocks)
	{
		std::snprintf(header.blockName, sizeof(header.blockName), "%s", name.c_str());
		header.blockSize = static_cast<uint32_t>(contents.size() * sizeof(contents[0]));

		stream.write(reinterpret_cast<const char*>(&header), sizeof(header));
		stream.write(reinterpret_cast<const char*>(contents.data()), header.blockSize);
	}
}

void PackFile::CreateTextureBlocks()
{
	// TODO(bwrsandman): Loop through every texture and create a block with
	//                   their id. Then fill in the look-up table.
	assert(_isLoaded);
	assert(false);
}

void PackFile::CreateRawBlock(const std::string& name, std::vector<uint8_t>&& data)
{
	if (HasBlock(name))
	{
		Fail("Pack file already has a " + name + " block");
	}

	_blocks[name] = std::move(data);
}

void PackFile::CreateMeshBlock()
{
	if (HasBlock("MESHES"))
	{
		Fail("Mesh pack already has a MESHES block");
	}

	size_t offset = 0;
	std::vector<uint8_t> contents;

	auto meshCount = static_cast<uint32_t>(_meshes.size());
	contents.resize(sizeof(kBlockMagic) + sizeof(meshCount));

	std::memcpy(contents.data() + offset, kBlockMagic, sizeof(kBlockMagic));
	offset += sizeof(kBlockMagic);
	std::memcpy(contents.data() + offset, &meshCount, sizeof(meshCount));
	offset += sizeof(meshCount);

	contents.resize(offset + sizeof(uint32_t) * meshCount);
	if (meshCount > 0)
	{
		auto meshOffsets = reinterpret_cast<uint32_t*>(&contents[offset]);
		offset += sizeof(uint32_t) * meshCount;
		for (size_t i = 0; i < _meshes.size(); ++i)
		{
			meshOffsets[i] = static_cast<uint32_t>(offset);
			offset += _meshes[i].size();
		}
		contents.resize(offset);
		for (size_t i = 0; i < _meshes.size(); ++i)
		{
			std::memcpy(&contents[meshOffsets[i]], _meshes[i].data(), _meshes[i].size() * sizeof(_meshes[i][0]));
		}
	}

	_blocks["MESHES"] = std::move(contents);
}

void PackFile::CreateInfoBlock()
{
	if (HasBlock("INFO"))
	{
		Fail("Mesh pack already has an INFO block");
	}

	uint32_t offset = 0;
	std::vector<uint8_t> contents;

	auto totalTextures = static_cast<uint32_t>(_infoBlockLookup.size());
	contents.resize(sizeof(totalTextures) + _infoBlockLookup.size() * sizeof(_infoBlockLookup[0]));

	std::memcpy(contents.data() + offset, &totalTextures, sizeof(totalTextures));
	offset += static_cast<uint32_t>(sizeof(totalTextures));

	std::memcpy(contents.data() + offset, _infoBlockLookup.data(), _infoBlockLookup.size() * sizeof(_infoBlockLookup[0]));

	_blocks["INFO"] = std::move(contents);
}

void PackFile::CreateBodyBlock()
{
	if (HasBlock("Body"))
	{
		Fail("Pack already has an Body block");
	}

	std::vector<uint8_t> contents;

	_blocks["Body"] = std::move(contents);
}

PackFile::PackFile()
    : _isLoaded(false)
{
}

void PackFile::Open(const std::string& file)
{
	_filename = file;
	ReadBlocks();
	// Mesh pack
	if (HasBlock("INFO"))
	{
		ResolveInfoBlock();
		ExtractTexturesFromBlock();
		ResolveMeshBlock();
	}
	// Anim pack
	if (HasBlock("Body"))
	{
		ResolveBodyBlock();
		ExtractAnimationsFromBlock();
	}

	_isLoaded = true;
}

void PackFile::Write(const std::string& file)
{
	assert(!_isLoaded);

	_filename = file;

	std::ofstream stream(_filename, std::ios::binary);

	if (!stream.is_open())
	{
		Fail("Could not open file.");
	}

	WriteBlocks(stream);
}

std::unique_ptr<std::istream> PackFile::GetBlockAsStream(const std::string& name) const
{
	auto& data = GetBlock(name);
	return std::make_unique<imemstream>(reinterpret_cast<const char*>(data.data()), data.size());
}
