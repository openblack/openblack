/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "AudioSample.h"
#include <istream>
#include <map>
#include <memory>
#include <streambuf>
#include <string>
#include <vector>

namespace openblack::sad
{

struct SadAudioMetaDataHeader
{
	char unknown[27];
	uint32_t entries;
};

struct SadAudioBankSampleTable
{
	uint16_t numOfEntries;
	uint16_t numOfEntries2;

};

struct SadBlockHeader
{
	char name[31];
	uint32_t size;
};

using SoundData = std::vector<uint8_t>;
using SoundEntry = std::tuple<AudioBankSample, std::vector<uint8_t>>;
using Sounds = std::vector<SoundEntry>;

/**
  This class is used to read LionHead sad files
 */
class SadFile
{
protected:
	static constexpr const char kMagic[8] = {'L', 'i', 'O', 'n', 'H', 'e', 'A', 'd'};

	/// True when a file has been loaded
	bool _isLoaded;

	std::string _filename;
	std::string _name;

	std::map<std::string, std::vector<uint8_t>> _blocks;
	Sounds _sounds;
	bool _isMusic;

	/// Error handling
	void Fail(const std::string& msg);

	void ResolveFileSegmentDataBlock();
	void ResolveAudioWaveDataBlock();
public:
	SadFile();

	virtual ~SadFile() = default;

	/// Read sad file from the filesystem
	void Open(const std::string& file);

	/// Read blocks from pack
	virtual void ReadBlocks();

	[[nodiscard]] const std::string& GetFilename() const { return _filename; }
	[[nodiscard]] const std::string& GetName() const { return _name; }
	[[nodiscard]] const std::map<std::string, std::vector<uint8_t>>& GetBlocks() const { return _blocks; }
	[[nodiscard]] bool HasBlock(const std::string& name) const { return _blocks.count(name); }
	[[nodiscard]] const std::vector<uint8_t>& GetBlock(const std::string& name) const { return _blocks.at(name); }
	[[nodiscard]] const Sounds GetSounds() const { return _sounds; }
};
}