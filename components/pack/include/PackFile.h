/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
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

template <typename N>
class Span
{
	const std::vector<N>& original;
	const size_t start;
	const size_t length;

public:
	Span(std::vector<N>& original, size_t start, size_t length)
	    : original(original)
	    , start(start)
	    , length(length)
	{
	}

	const N* data() const noexcept { return original.data() + start; }

	typename std::vector<N>::const_reference operator[](typename std::vector<N>::size_type index) const noexcept
	{
		return original[index + start];
	}

	constexpr typename std::vector<N>::size_type size() const noexcept { return length; }

	// First element.
	constexpr typename std::vector<N>::const_iterator begin() const noexcept { return original.begin() + start; }

	// One past the last element.
	constexpr typename std::vector<N>::const_iterator end() const noexcept { return begin() + size(); }
};

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
struct AudioBankSample
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

static_assert(sizeof(AudioBankSample) == 0x280, "Audio sample header has incorrect size");

struct SadBankInfo
{
	uint32_t unknown_0x0;
	uint32_t unknown_0x4;
	uint32_t musicBank;
	std::array<char, 0x104> description;
	std::array<char, 0x104> backwardsComment;
};

struct Sound
{
	AudioBankSample sample;
	std::vector<uint8_t> buffer;
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
	/// Audio file segment info
	SadBankInfo _audioInfo;
	/// Bytes of audio bank samples
	std::vector<AudioBankSample> _audioSamples;
	/// Bytes of sounds
	std::vector<Sound> _sounds;

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

	/// Parse Audio Block for sound pack
	virtual void ResolveAudioBankSampleBlock();

	/// Parse the Audio file segement bank block
	virtual void ResolveFileSegmentBankBlock();

	/// Extract Textures from all Blocks named in INFO Block
	virtual void ExtractTexturesFromBlock();

	/// Extract Animations from all Blocks named in Body Block
	virtual void ExtractAnimationsFromBlock();

	/// Extract Sounds from all Blocks named in LHAudioBankSampleTable Block
	virtual void ExtractSoundsFromBlock();

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
	[[nodiscard]] bool HasBlock(const std::string& name) const { return _blocks.contains(name); }
	[[nodiscard]] const std::vector<uint8_t>& GetBlock(const std::string& name) const { return _blocks.at(name); }
	[[nodiscard]] std::unique_ptr<std::istream> GetBlockAsStream(const std::string& name) const;
	[[nodiscard]] const std::vector<InfoBlockLookup>& GetInfoBlockLookup() const { return _infoBlockLookup; }
	[[nodiscard]] const std::vector<BodyBlockLookup>& GetBodyBlockLookup() const { return _bodyBlockLookup; }
	[[nodiscard]] const std::map<std::string, G3DTexture>& GetTextures() const { return _textures; }
	[[nodiscard]] const G3DTexture& GetTexture(const std::string& name) const { return _textures.at(name); }
	[[nodiscard]] const std::vector<std::vector<uint8_t>>& GetMeshes() const { return _meshes; }
	[[nodiscard]] const std::vector<uint8_t>& GetMesh(uint32_t index) const { return _meshes[index]; }
	[[nodiscard]] const std::vector<std::vector<uint8_t>>& GetAnimations() const { return _animations; }
	[[nodiscard]] const std::vector<Sound>& GetSounds() const { return _sounds; }
	[[nodiscard]] const std::vector<uint8_t>& GetAnimation(uint32_t index) const { return _animations[index]; }
};

} // namespace openblack::pack
