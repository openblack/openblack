/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/
// TODO(bwrsandman): rename to packtool?

#pragma once

#include <istream>
#include <map>
#include <memory>
#include <streambuf>
#include <string>
#include <vector>

namespace openblack::g3d
{

struct G3DInfoBlockLookup
{
	uint32_t blockId;
	uint32_t unknown;
};
static_assert(sizeof(G3DInfoBlockLookup) == 8);

struct G3DBodyBlockLookup
{
	uint32_t offset;
	uint32_t unknown; // TODO
};
static_assert(sizeof(G3DBodyBlockLookup) == 8);

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
	char fourCC[4];
	uint32_t bitCount;
	uint32_t rBitMask;
	uint32_t gBitMask;
	uint32_t bBitMask;
	uint32_t aBitMask;
};

struct DdsCapabilities2
{
	uint32_t caps[2];
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
	uint32_t reserved1[11];
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
class G3DFile
{
protected:
	static constexpr const char kMagic[8] = {'L', 'i', 'O', 'n', 'H', 'e', 'A', 'd'};

	/// True when a file has been loaded
	bool _isLoaded;

	std::string _filename;

	std::map<std::string, std::vector<uint8_t>> _blocks;
	std::vector<G3DInfoBlockLookup> _infoBlockLookup;
	std::vector<G3DBodyBlockLookup> _bodyBlockLookup;
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

	/// Create Texture Blocks from textures
	void CreateTextureBlocks();

	/// Create Mesh Block from meshes
	void CreateMeshBlock();

	/// Create Info block from look-up table
	void CreateInfoBlock();

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
	G3DFile();

	virtual ~G3DFile() = default;

	/// Read l3d file from the filesystem
	void Open(const std::string& file);

	/// Write g3d file to path on the filesystem
	void Write(const std::string& file);

	[[nodiscard]] const std::string& GetFilename() const { return _filename; }
	[[nodiscard]] const std::map<std::string, std::vector<uint8_t>>& GetBlocks() const { return _blocks; }
	[[nodiscard]] bool HasBlock(const std::string& name) const { return _blocks.count(name); }
	[[nodiscard]] const std::vector<uint8_t>& GetBlock(const std::string& name) const { return _blocks.at(name); }
	[[nodiscard]] std::unique_ptr<std::istream> GetBlockAsStream(const std::string& name) const;
	[[nodiscard]] const std::vector<G3DInfoBlockLookup>& GetInfoBlockLookup() const { return _infoBlockLookup; }
	[[nodiscard]] const std::vector<G3DBodyBlockLookup>& GetBodyBlockLookup() const { return _bodyBlockLookup; }
	[[nodiscard]] const std::map<std::string, G3DTexture>& GetTextures() const { return _textures; }
	[[nodiscard]] const G3DTexture& GetTexture(const std::string& name) const { return _textures.at(name); }
	[[nodiscard]] const std::vector<std::vector<uint8_t>>& GetMeshes() const { return _meshes; }
	[[nodiscard]] const std::vector<uint8_t>& GetMesh(uint32_t index) const { return _meshes[index]; }
	[[nodiscard]] const std::vector<std::vector<uint8_t>>& GetAnimations() const { return _animations; }
	[[nodiscard]] const std::vector<uint8_t>& GetAnimation(uint32_t index) const { return _animations[index]; }
};

} // namespace openblack::g3d
