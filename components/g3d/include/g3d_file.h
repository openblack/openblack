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
  This class is used to read LionHead Mesh Packs or G3Ds.
 */
class G3DFile
{
public:
	static constexpr const char kMagic[8] = {'L', 'i', 'O', 'n', 'H', 'e', 'A', 'd'};

protected:
	/// True when a file has been loaded
	bool _isLoaded;

	std::string _filename;

	std::map<std::string, std::vector<uint8_t>> _blocks;
	std::vector<G3DInfoBlockLookup> _infoBlockLookup;
	/// Metadata and DDS formatted texture data
	std::map<std::string, G3DTexture> _textures;
	/// Bytes of l3d meshes
	std::vector<std::vector<uint8_t>> _meshes;

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

	/// Parse Info Block
	virtual void ResolveInfoBlock();

	/// Extract Textures from all Blocks
	virtual void ExtractTextureFromBlock();

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
	[[nodiscard]] const std::map<std::string, G3DTexture>& GetTextures() const { return _textures; }
	[[nodiscard]] const G3DTexture& GetTexture(const std::string& name) const { return _textures.at(name); }
	[[nodiscard]] const std::vector<std::vector<uint8_t>>& GetMeshes() const { return _meshes; }
	[[nodiscard]] const std::vector<uint8_t>& GetMesh(uint32_t index) const { return _meshes[index]; }
};

} // namespace openblack::g3d
