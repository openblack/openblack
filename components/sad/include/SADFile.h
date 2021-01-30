/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <array>
#include <cstdint>
#if __cplusplus > 201703L
#include <span>
#endif
#include <string>
#include <tuple>
#include <vector>

namespace openblack::sad
{

// TODO(bwrsandman): If you read this in c++20, replace with std::span
#if __cplusplus > 201703L
template <typename T>
using Span = std::span<T>;
#else
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
#endif

struct SadBankInfo
{
	uint32_t unknown_0x0;
	uint32_t unknown_0x4;
	uint32_t musicBank;
	std::array<char, 0x104> description;
	std::array<char, 0x104> backwardsComment;
};

enum class AudioBankLoop : uint16_t
{
	None,
	Restart,
	Once,
	Overlap,
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

/**
  This class is used to the segments of a SAD file.
 */
class SADFile
{
protected:
	/// True when a file has been loaded
	std::array<bool, 3> _isLoaded;

	std::tuple<std::string, std::string, std::string> _filenames;

	SadBankInfo _info;
	std::vector<AudioBankSample> _table;
	std::vector<uint8_t> _soundBuffer;
	std::vector<Span<uint8_t>> _sounds;

	/// Error handling
	void Fail(const std::string& msg);

	/// Read info stream from the input source
	virtual void ReadInfoStream(std::istream& stream);
	/// Read table stream from the input source
	virtual void ReadTableStream(std::istream& stream);
	/// Read data stream from the input source
	virtual void ReadDataStream(std::istream& stream);

public:
	SADFile();
	virtual ~SADFile();

	/// Read sad file from the filesystem
	void Open(const std::string& infoFilename, const std::string& tableFilename, const std::string& dataFilename);
	/// Read sad file from buffers
	void Open(const std::vector<uint8_t>& infoBuffer, const std::vector<uint8_t>& tableBuffer,
	          const std::vector<uint8_t>& dataBuffer);

	[[nodiscard]] const auto& GetFilenames() const { return _filenames; }
	[[nodiscard]] const auto& GetInfo() const { return _info; }
	[[nodiscard]] const auto& GetTable() const { return _table; }
	[[nodiscard]] const auto& GetSoundData(uint32_t i) const { return _sounds[i]; }
};

} // namespace openblack::sad
