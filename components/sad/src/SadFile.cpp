/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <SadFile.h>

#ifdef HAS_FILESYSTEM
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif // HAS_FILESYSTEM

#include <cassert>
#include <cstring>
#include <fstream>
#include <ios>

using namespace openblack::sad;

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

SadFile::SadFile()
    : _isLoaded(false)
{
}
void SadFile::Open(const std::string& file)
{
	_filename = file;
	ReadBlocks();
	ResolveFileSegmentDataBlock();
	ResolveAudioWaveDataBlock();
	// Mesh pack
	_isLoaded = true;
	_isMusic = false;
}
void SadFile::ReadBlocks()
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
	if (fsize < sizeof(magic) + sizeof(SadBlockHeader))
	{
		Fail("File too small to be a valid Pack file.");
	}

	// First 8 bytes
	input.read(reinterpret_cast<char*>(&magic), sizeof(magic));
	if (std::memcmp(&magic, kMagic, sizeof(magic)) != 0)
	{
		Fail("Unrecognized Pack header");
	}

	if (fsize < static_cast<std::size_t>(input.tellg()) + sizeof(SadBlockHeader))
	{
		Fail("File too small to contain any blocks.");
	}

	SadBlockHeader header;
	while (fsize - sizeof(SadBlockHeader) > static_cast<std::size_t>(input.tellg()))
	{
		input.read(reinterpret_cast<char*>(&header), sizeof(SadBlockHeader));

		if (_blocks.count(header.name) > 0)
		{
			Fail(std::string("Duplicate block name: ") + header.name);
		}

		_blocks[std::string(header.name)] = std::vector<uint8_t>(header.size);
		input.read(reinterpret_cast<char*>(_blocks[header.name].data()), header.size);
	}

	if (fsize < static_cast<std::size_t>(input.tellg()))
	{
		Fail("File not evenly split into whole blocks.");
	}
}
void SadFile::ResolveFileSegmentDataBlock()
{
	if (!HasBlock("LHFileSegmentBankInfo"))
	{
		Fail("no LHAudioBankSampleTable block in sad file");
	}

	auto tableData = GetBlock("LHFileSegmentBankInfo");
	imemstream stream(reinterpret_cast<const char*>(tableData.data()), tableData.size());
	stream.seekoff(12, std::ios_base::cur, std::ios_base::out);
	char name[255];
	stream.read(name, 255);
	_name = name;
	// TODO: Discover header data for Music Bank tickbox in the Lionhead Sound Bank Maker
	_isMusic = false;
}

void SadFile::ResolveAudioWaveDataBlock()
{
	if (!HasBlock("LHAudioBankSampleTable"))
	{
		Fail("no LHAudioBankSampleTable block in sad file");
	}

	SadAudioBankSampleTable table;
	auto tableData = GetBlock("LHAudioBankSampleTable");
	imemstream tableStream(reinterpret_cast<const char*>(tableData.data()), tableData.size());
	tableStream.read(reinterpret_cast<char*>(&table), sizeof(table));

	if (!HasBlock("LHAudioWaveData"))
	{
		Fail("No LHAudioBankSampleTable block in sad file");
	}

	if (table.numOfEntries == 0)
	{
		Fail("Audio bank has no sound entries");
	}

	std::vector<AudioBankSample> soundOffsets;

	for (uint32_t i = 0; i < table.numOfEntries; i++)
	{
		AudioBankSample sample;

		tableStream.read(reinterpret_cast<char*>(&sample), sizeof(sample));
		std::string fileName = sample.audioSampleName;

		// TODO: Warn?
		if (sample.size == 0)
		{
			continue;
		}

		soundOffsets.push_back(sample);
	}

	auto entryData = GetBlock("LHAudioWaveData");
	imemstream entryStream(reinterpret_cast<const char*>(entryData.data()), entryData.size());

	for (auto sample : soundOffsets)
	{
		std::string filePath = sample.audioSampleName;
		// Remove drive letter and make path relative
		filePath.erase(0, 2);

		entryStream.seekpos(sample.relativeOffset);
		std::string fileName = fs::path(filePath).filename().string();

		if (fileName.empty())
		{
			Fail("Empty file name for sound entry");
		}

		if ((entryData.size() - entryStream.tellg()) < sample.size)
		{
			Fail("Sound sample size exceeds LHAudioWaveData size");
		}

		SoundData sound(sample.size);
		entryStream.read(reinterpret_cast<char*>(sound.data()), sound.size());
		_sounds.push_back(std::make_tuple(sample, std::move(sound)));
	}
}

void SadFile::Fail(const std::string& msg)
{
	throw std::runtime_error("Sad Error: " + msg + "\nFilename: " + _filename);
}
