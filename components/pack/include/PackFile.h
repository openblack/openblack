/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

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
	uint16_t priority; ///< 0-9999
	uint16_t unknown9; ///<
	/// Which play parameters this header overrides (LHSamplePlay applies a header value only when its bit is set
	/// here and the caller did not set the same bit in its play options).
	uint32_t overrideFlags;
	int32_t loop;             ///< Loop count (-1 = forever), applied with AudioBankOverride::Loop
	int32_t pan;              ///< Applied with AudioBankOverride::Pan
	std::array<float, 3> pos; ///< -9999 to 9999
	uint16_t volume;          ///< 0-127, applied with AudioBankOverride::Volume
	uint16_t userParam;       ///<
	uint32_t pitch;           ///< Playback rate in percent, applied with AudioBankOverride::Pitch
	uint32_t pitchDeviation;  ///< Random +/- percentage applied to the playback rate on every play
	float minDist;            ///<
	float maxDist;            ///<
	float scale;              ///< QSound distance scale
	AudioBankLoop loopType;   ///<
	uint16_t unknown21;       ///<
	uint16_t unknown22;       ///<
	uint16_t unknown23;       ///<
	/// Atmos banks only: 0 = looping bed, >0 = one-shot whose mean retrigger interval is 10 * atmosInterval game
	/// turns, <0 = not part of the atmosphere
	int32_t atmosInterval;
};
static_assert(sizeof(AudioBankSampleHeader) == 0x280);
static_assert(offsetof(AudioBankSampleHeader, id) == 0x104);
static_assert(offsetof(AudioBankSampleHeader, atmosGroup) == 0x11A);
static_assert(offsetof(AudioBankSampleHeader, overrideFlags) == 0x244);
static_assert(offsetof(AudioBankSampleHeader, volume) == 0x25C);
static_assert(offsetof(AudioBankSampleHeader, pitch) == 0x260);
static_assert(offsetof(AudioBankSampleHeader, minDist) == 0x268);
static_assert(offsetof(AudioBankSampleHeader, atmosInterval) == 0x27C);

/// Bits of AudioBankSampleHeader::overrideFlags and of the matching play options mask
enum class AudioBankOverride : uint32_t
{
	Pitch = 0x1,
	Pan = 0x2,
	PosX = 0x4,
	PosY = 0x8,
	PosZ = 0x10,
	Volume = 0x20,
	Loop = 0x40,
	MinDist = 0x80,
	MaxDist = 0x100,
	Scale = 0x200,
	LoopType = 0x400,
};

/// First 3 u32 of the LHFileSegmentBankInfo block of a .sad (532 bytes: 3 u32 + a 520 byte title). The game keeps the
/// third, which marks a music bank: 1 in every .sad of Audio\Music and in Dialogue\MissionariesVerse1..3.sad, 0 in the
/// rest.
struct AudioBankInfo
{
	uint32_t unknown0; ///< 0 except in SFX\Atmos\ocean.sad (7) (unknown)
	uint32_t unknown1; ///< 0 except in SFX\Atmos\ocean.sad (6) (unknown)
	uint32_t isMusic;  ///< non-zero = music bank
};
static_assert(sizeof(AudioBankInfo) == 3 * sizeof(uint32_t));

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
	/// High word of the LHAudioBankSampleTable header: non-zero marks an atmosphere bank
	uint16_t _audioBankAtmosCount {0};
	/// Start of the LHFileSegmentBankInfo block of a sound pack (all zero if the pack has none)
	AudioBankInfo _audioBankInfo {};

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

	/// Parse the LHFileSegmentBankInfo block of a sound pack
	PackResult ResolveFileSegmentBankInfoBlock() noexcept;

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
	[[nodiscard]] uint16_t GetAudioBankAtmosCount() const noexcept { return _audioBankAtmosCount; }
	[[nodiscard]] const AudioBankInfo& GetAudioBankInfo() const noexcept { return _audioBankInfo; }
	/// Whether the pack is a music bank, which only music may play from
	[[nodiscard]] bool IsAudioMusicBank() const noexcept { return _audioBankInfo.isMusic != 0; }
	[[nodiscard]] const std::vector<uint8_t>& GetAudioSampleData(uint32_t index) const noexcept
	{
		return _audioSampleData[index];
	}
};

} // namespace openblack::pack
