/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

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

enum class PackResult : uint8_t
{
	Success = 0,
	ErrCantOpen,
	ErrFileTooSmall,
	ErrNoEntries,
	ErrUnrecognizedHeader,
	ErrUnrecognizedBlockHeader,
	ErrFileNotEvenlySplit,
	ErrDuplicateBlockName,
	ErrMissingInfoBlock,
	ErrMissingBodyBlock,
	ErrMissingAudioBankSampleTableBlock,
	ErrMissingAudioWaveDataBlock,
	ErrMissingTextureBlock,
	ErrTextureBlockIdMismatch,
	ErrTextureDuplicate,
	ErrTextureInvalidDDSHeaderSize,
	ErrMissingMeshBlock,
	ErrMeshBlockHeaderMalformed,
	ErrNotImplemented,
};

std::string_view ResultToStr(PackResult result);

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

enum class AudioBankLoop : uint16_t
{
	None,
	Restart,
	Once,
	Overlap,
};

enum class ChannelLayout
{
	Mono,
	Stereo
};

// TODO(raffclar): Look for channel count (e.g 2)
// TODO(raffclar): Look for word length (e.g 16);
struct AudioBankSampleHeader
{
	std::array<char, 0x100> name;
	int32_t unknown0;
	int32_t id;
	int32_t isBank;
	uint32_t size;
	uint32_t offset;
	int32_t isClone;
	int16_t group;
	int16_t atmosGroup;
	int32_t unknown4;
	int32_t unknown5;
	int16_t unknown6a;
	int16_t unknown6b;
	uint32_t sampleRate;
	int16_t unknownOthera;
	int16_t unknownOtherb;
	int16_t unknown7a;
	int16_t unknown7b;
	int32_t unknown8;
	int32_t lStart;
	int32_t lEnd;
	std::array<char, 0x100> description;
	uint16_t priority;        ///< 0-9999
	uint16_t unknown9;        ///<
	uint16_t unknown10;       ///<
	uint16_t unknown11;       ///<
	int16_t loop;             ///<
	uint16_t start;           ///<
	uint8_t pan;              ///<
	uint16_t unknown12;       ///<
	std::array<float, 3> pos; ///< -9999 to 9999
	uint8_t volume;           ///<
	uint16_t userParam;       ///<
	uint16_t pitch;           ///<
	uint16_t unknown18;       ///<
	uint16_t pitchDeviation;  ///<
	uint16_t unknown20;       ///<
	float minDist;            ///<
	float maxDist;            ///<
	float scale;              ///< 0-50 (multiply by 10)
	AudioBankLoop loopType;   ///<
	uint16_t unknown21;       ///<
	uint16_t unknown22;       ///<
	uint16_t unknown23;       ///<
	uint16_t atmos;           ///<
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

	std::map<std::string, std::vector<uint8_t>> _blocks;
	std::vector<InfoBlockLookup> _infoBlockLookup;
	std::vector<BodyBlockLookup> _bodyBlockLookup;
	/// Metadata and DDS formatted texture data
	std::map<std::string, G3DTexture> _textures;
	/// Bytes of l3d meshes
	std::vector<std::vector<uint8_t>> _meshes;
	/// Bytes of anm meshes
	std::vector<std::vector<uint8_t>> _animations;
	/// Headers of snd audio samples
	std::vector<AudioBankSampleHeader> _audioSampleHeaders;
	/// Bytes of snd audio samples
	std::vector<std::vector<uint8_t>> _audioSampleData;

	/// Read blocks from pack
	PackResult ReadBlocks(std::istream& stream) noexcept;

	/// Write blocks to file
	PackResult WriteBlocks(std::ostream& stream) const noexcept;

	/// Parse Info Block for mesh pack
	PackResult ResolveInfoBlock() noexcept;

	/// Parse Body Block for anim pack
	PackResult ResolveBodyBlock() noexcept;

	/// Parse Audio Block for sound pack
	PackResult ResolveAudioBankSampleTableBlock() noexcept;

	/// Extract Textures from all Blocks named in INFO Block
	PackResult ExtractTexturesFromBlock() noexcept;

	/// Extract Animations from all Blocks named in Body Block
	PackResult ExtractAnimationsFromBlock() noexcept;

	/// Extract Sounds from all Blocks named in LHAudioBankSampleTable Block
	PackResult ExtractSoundsFromBlock() noexcept;

	/// Parse Info Block
	PackResult ResolveMeshBlock() noexcept;

public:
	PackFile() noexcept;
	virtual ~PackFile() noexcept;

	/// Read file from the input source
	PackResult ReadFile(std::istream& stream) noexcept;

	/// Read g3d file from the filesystem
	PackResult Open(const std::filesystem::path& filepath) noexcept;

	/// Read g3d file from a buffer
	PackResult Open(const std::vector<uint8_t>& buffer) noexcept;

	/// Write pack file to path on the filesystem
	PackResult Write(const std::filesystem::path& filepath) noexcept;

	/// Create Texture Blocks from textures
	PackResult CreateTextureBlocks() noexcept;

	/// Create Data Block from Raw Data
	PackResult CreateRawBlock(const std::string& name, std::vector<uint8_t>&& data) noexcept;

	/// Create Mesh Block from meshes
	PackResult CreateMeshBlock() noexcept;

	/// Insert Mesh Data for the Mesh Block
	PackResult InsertMesh(std::vector<uint8_t> data) noexcept;

	/// Create Info block from look-up table
	PackResult CreateInfoBlock() noexcept;

	/// Create Body block from look-up table
	PackResult CreateBodyBlock() noexcept;

	[[nodiscard]] const std::map<std::string, std::vector<uint8_t>>& GetBlocks() const noexcept { return _blocks; }
	[[nodiscard]] bool HasBlock(const std::string& name) const noexcept { return _blocks.contains(name); }
	[[nodiscard]] const std::vector<uint8_t>& GetBlock(const std::string& name) const noexcept { return _blocks.at(name); }
	[[nodiscard]] std::unique_ptr<std::istream> GetBlockAsStream(const std::string& name) const noexcept;
	[[nodiscard]] const std::vector<InfoBlockLookup>& GetInfoBlockLookup() const noexcept { return _infoBlockLookup; }
	[[nodiscard]] const std::vector<BodyBlockLookup>& GetBodyBlockLookup() const noexcept { return _bodyBlockLookup; }
	[[nodiscard]] const std::map<std::string, G3DTexture>& GetTextures() const noexcept { return _textures; }
	[[nodiscard]] const G3DTexture& GetTexture(const std::string& name) const noexcept { return _textures.at(name); }
	[[nodiscard]] const std::vector<std::vector<uint8_t>>& GetMeshes() const noexcept { return _meshes; }
	[[nodiscard]] const std::vector<uint8_t>& GetMesh(uint32_t index) const noexcept { return _meshes[index]; }
	[[nodiscard]] const std::vector<std::vector<uint8_t>>& GetAnimations() const noexcept { return _animations; }
	[[nodiscard]] const std::vector<uint8_t>& GetAnimation(uint32_t index) const noexcept { return _animations[index]; }
	[[nodiscard]] const std::vector<AudioBankSampleHeader>& GetAudioSampleHeaders() const noexcept
	{
		return _audioSampleHeaders;
	}
	[[nodiscard]] const AudioBankSampleHeader& GetAudioSampleHeader(uint32_t index) const noexcept
	{
		return _audioSampleHeaders[index];
	}
	[[nodiscard]] const std::vector<std::vector<uint8_t>>& GetAudioSamplesData() const noexcept { return _audioSampleData; }
	[[nodiscard]] const std::vector<uint8_t>& GetAudioSampleData(uint32_t index) const noexcept
	{
		return _audioSampleData[index];
	}
};

} // namespace openblack::pack
