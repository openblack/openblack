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
#include <istream>
#include <map>
#include <memory>
#include <streambuf>
#include <string>
#include <vector>

namespace openblack::pack
{

struct InfoBlockLookup
{
	uint32_t blockId;
	uint32_t unknown;
};
static_assert(sizeof(InfoBlockLookup) == 8);

struct BodyBlockLookup
{
	uint32_t offset;
	uint32_t unknown; // TODO(#457)
};
static_assert(sizeof(BodyBlockLookup) == 8);

struct G3DTextureHeader
{
	uint32_t size;
	uint32_t id;
	uint32_t type;
	uint32_t ddsSize;
};

static_assert(sizeof(G3DTextureHeader) == 4 * sizeof(uint32_t));

struct DdsPixelFormat
{
	uint32_t size;
	uint32_t flags;
	std::array<char, 4> fourCC;
	uint32_t bitCount;
	uint32_t rBitMask;
	uint32_t gBitMask;
	uint32_t bBitMask;
	uint32_t aBitMask;
};

struct DdsCapabilities2
{
	std::array<uint32_t, 2> caps;
	uint32_t ddsx;
	uint32_t reserved;
};

struct DdsHeader
{
	uint32_t size;
	uint32_t flags;
	uint32_t height;
	uint32_t width;
	uint32_t pitchOrLinearSize;
	uint32_t depth;
	uint32_t mipMapCount;
	std::array<uint32_t, 11> reserved1;
	DdsPixelFormat format;
	DdsCapabilities2 capabilities;
	uint32_t reserved2;
};

struct G3DTexture
{
	G3DTextureHeader header;
	DdsHeader ddsHeader;
	std::vector<uint8_t> ddsData;
};

/**
  This class is used to read LionHead Packs files
 */
class PackFile
{
protected:
	static constexpr const std::array<char, 8> k_Magic = {'L', 'i', 'O', 'n', 'H', 'e', 'A', 'd'};

	/// True when a file has been loaded
	bool _isLoaded {false};

	std::filesystem::path _filename;

	std::map<std::string, std::vector<uint8_t>> _blocks;
	std::vector<InfoBlockLookup> _infoBlockLookup;
	std::vector<BodyBlockLookup> _bodyBlockLookup;
	/// Metadata and DDS formatted texture data
	std::map<std::string, G3DTexture> _textures;
	/// Bytes of l3d meshes
	std::vector<std::vector<uint8_t>> _meshes;
	/// Bytes of anm meshes
	std::vector<std::vector<uint8_t>> _animations;

	/// Error handling
	void Fail(const std::string& msg);

	/// Read blocks from pack
	virtual void ReadBlocks();

	/// Write blocks to file
	virtual void WriteBlocks(std::ostream& stream) const;

	/// Parse Info Block for mesh pack
	virtual void ResolveInfoBlock();

	/// Parse Body Block for anim pack
	virtual void ResolveBodyBlock();

	/// Extract Textures from all Blocks named in INFO Block
	virtual void ExtractTexturesFromBlock();

	/// Extract Animations from all Blocks named in Body Block
	virtual void ExtractAnimationsFromBlock();

	/// Parse Info Block
	virtual void ResolveMeshBlock();

public:
	PackFile();
	virtual ~PackFile();

	/// Read l3d file from the filesystem
	void Open(const std::filesystem::path& file);

	/// Write pack file to path on the filesystem
	void Write(const std::filesystem::path& file);

	/// Create Texture Blocks from textures
	void CreateTextureBlocks();

	/// Create Data Block from Raw Data
	void CreateRawBlock(const std::string& name, std::vector<uint8_t>&& data);

	/// Create Mesh Block from meshes
	void CreateMeshBlock();

	/// Insert Mesh Data for the Mesh Block
	void InsertMesh(std::vector<uint8_t> data);

	/// Create Info block from look-up table
	void CreateInfoBlock();

	/// Create Body block from look-up table
	void CreateBodyBlock();

	[[nodiscard]] std::string GetFilename() const { return _filename.string(); }
	[[nodiscard]] const std::map<std::string, std::vector<uint8_t>>& GetBlocks() const { return _blocks; }
	[[nodiscard]] bool HasBlock(const std::string& name) const { return _blocks.count(name) != 0u; }
	[[nodiscard]] const std::vector<uint8_t>& GetBlock(const std::string& name) const { return _blocks.at(name); }
	[[nodiscard]] std::unique_ptr<std::istream> GetBlockAsStream(const std::string& name) const;
	[[nodiscard]] const std::vector<InfoBlockLookup>& GetInfoBlockLookup() const { return _infoBlockLookup; }
	[[nodiscard]] const std::vector<BodyBlockLookup>& GetBodyBlockLookup() const { return _bodyBlockLookup; }
	[[nodiscard]] const std::map<std::string, G3DTexture>& GetTextures() const { return _textures; }
	[[nodiscard]] const G3DTexture& GetTexture(const std::string& name) const { return _textures.at(name); }
	[[nodiscard]] const std::vector<std::vector<uint8_t>>& GetMeshes() const { return _meshes; }
	[[nodiscard]] const std::vector<uint8_t>& GetMesh(uint32_t index) const { return _meshes[index]; }
	[[nodiscard]] const std::vector<std::vector<uint8_t>>& GetAnimations() const { return _animations; }
	[[nodiscard]] const std::vector<uint8_t>& GetAnimation(uint32_t index) const { return _animations[index]; }
};

} // namespace openblack::pack
