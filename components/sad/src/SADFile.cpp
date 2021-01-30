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
 *
 * The layout of a SAD File is split into three buffers as follows:
 *
 * - 532 byte info segment containing
 *         8 bytes - always set to zero except for ocean.sad
 *         4 bytes bool - music bank
 *         a 260 byte string - description
 *         a 260 byte string - optional comment with character order reversed
 *
 * - variable size table segment containing
 *         - 2 byte sample count (see below)
 *         - 2 byte sample count (again?)
 *
 * - variable size wave data segment containing
 *         - continuous byte data defined in the table segment
 *
 * ------------------------ start of sample block ------------------------------
 * - 640 bytes * sample count, each record contains:
 *         name - 256 character long string
 *         unknown 4 bytes
 *         id - 4 bytes int
 *         isBank - 4 bytes bool representing a group of sounds
 *         size - 4 bytes buffer size in the LHAudioWaveData buffer
 *         offset - 4 bytes buffer offset in the LHAudioWaveData buffer
 *         isClone - 4 bytes bool with unknown effect
 *         group - 2 bytes int
 *         atmosGroup - 2 bytes int
 *         2 * unknown 4 bytes
 *         2 * unknown 2 bytes
 *         sampleRate - 4 bytes int
 *         4 * unknown 2 bytes
 *         unknown 4 bytes
 *         lStart - 4 bytes int representing cropping start
 *         lEnd - 4 bytes int representing cropping end
 *         description - 256 character long string
 *         priority - 2 bytes int between 0 and 9999
 *         3 * unknown 2 bytes
 *         loop - 2 bytes bool
 *         start - 2 bytes int
 *         pan - 1 byte int
 *         unknown 2 bytes
 *         pos - 3 * 4 bytes float representing the position of sound between -9999 and 9999
 *         volume - 1 byte int
 *         userParam - 2 bytes int
 *         pitch - 2 bytes int
 *         unknown 2 bytes
 *         pitchDeviation - 2 bytes int
 *         unknown 2 bytes
 *         minDist - 4 bytes float
 *         maxDist - 4 bytes float
 *         scale - 4 bytes float
 *         loopType - 2 bytes enum
 *         3 * unknown 2 bytes
 *         atmos - 2 bytes int
 */

#include <SADFile.h>

#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <tuple>

using namespace openblack::sad;

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
struct imemstream: virtual membuf, std::istream
{
	imemstream(char const* base, size_t size)
	    : membuf(base, size)
	    , std::istream(dynamic_cast<std::streambuf*>(this))
	{
	}
};
} // namespace

/// Error handling
void SADFile::Fail(const std::string& msg)
{
	throw std::runtime_error("Sad Error: " + msg + "\nFilenames: " + std::get<0>(_filenames) + ", " + std::get<1>(_filenames) +
	                         ", " + std::get<2>(_filenames));
}

SADFile::SADFile() = default;

SADFile::~SADFile() = default;

void SADFile::Open(const std::string& infoFilename, const std::string& tableFilename, const std::string& dataFilename)
{
	assert(!_isLoaded[0] && !_isLoaded[1] && !_isLoaded[2]);

	if (!infoFilename.empty())
	{
		std::ifstream stream = std::ifstream(infoFilename, std::ios::binary);
		if (!stream.is_open())
		{
			Fail("Could not open info file.");
		}
		ReadInfoStream(stream);
	}
	std::get<0>(_filenames) = infoFilename;

	if (!tableFilename.empty())
	{
		std::ifstream stream = std::ifstream(tableFilename, std::ios::binary);
		if (!stream.is_open())
		{
			Fail("Could not open table file.");
		}
		ReadTableStream(stream);
	}
	std::get<1>(_filenames) = tableFilename;

	if (!dataFilename.empty())
	{
		std::ifstream stream = std::ifstream(dataFilename, std::ios::binary);
		if (!stream.is_open())
		{
			Fail("Could not open data file.");
		}
		ReadDataStream(stream);
	}
	std::get<2>(_filenames) = dataFilename;
}

void SADFile::Open(const std::vector<uint8_t>& infoBuffer, const std::vector<uint8_t>& tableBuffer,
                   const std::vector<uint8_t>& dataBuffer)
{
	assert(!_isLoaded[0] && !_isLoaded[1] && !_isLoaded[2]);

	imemstream infoStream(reinterpret_cast<const char*>(infoBuffer.data()), infoBuffer.size() * sizeof(infoBuffer[0]));
	imemstream tableStream(reinterpret_cast<const char*>(tableBuffer.data()), tableBuffer.size() * sizeof(tableBuffer[0]));
	imemstream dataStream(reinterpret_cast<const char*>(dataBuffer.data()), dataBuffer.size() * sizeof(dataBuffer[0]));

	_filenames = std::make_tuple("buffer", "buffer", "buffer");

	ReadInfoStream(infoStream);
	ReadTableStream(tableStream);
	ReadDataStream(dataStream);
}

void SADFile::ReadInfoStream(std::istream& stream)
{
	assert(!_isLoaded[0]);

	// Total stream size
	std::size_t fsize = 0;
	if (stream.seekg(0, std::ios_base::end))
	{
		fsize = static_cast<std::size_t>(stream.tellg());
		stream.seekg(0);
	}

	if (fsize != sizeof(_info))
	{
		Fail(std::string("Info block size does not match that of SadBankInfo: " + std::to_string(fsize) +
		                 " != " + std::to_string(sizeof(_info))));
	}

	// All 0x220 bytes
	stream.read(reinterpret_cast<char*>(&_info), sizeof(_info));

	_isLoaded[0] = true;
}

void SADFile::ReadTableStream([[maybe_unused]] std::istream& stream)
{
	assert(!_isLoaded[1]);

	// Total stream size
	std::size_t fsize = 0;
	if (stream.seekg(0, std::ios_base::end))
	{
		fsize = static_cast<std::size_t>(stream.tellg());
		stream.seekg(0);
	}

	if (fsize < sizeof(uint32_t))
	{
		Fail(std::string("Table block cannot fit sample count: " + std::to_string(fsize) + " < " +
		                 std::to_string(sizeof(uint8_t))));
	}

	std::pair<uint16_t, uint16_t> sampleCount;
	stream.read(reinterpret_cast<char*>(&sampleCount), sizeof(sampleCount));

	if (sampleCount.first == 0)
	{
		Fail("Audio bank has no sound entries");
	}

	_table.resize(sampleCount.first);

	if (fsize != sizeof(uint32_t) + _table.size() * sizeof(_table[0]))
	{
		Fail("Table block cannot fit all " + std::to_string(sampleCount.first) + " samples.");
	}

	stream.read(reinterpret_cast<char*>(_table.data()), _table.size() * sizeof(_table[0]));

	_isLoaded[1] = true;
}

void SADFile::ReadDataStream(std::istream& stream)
{
	assert(!_isLoaded[2]);

	if (!_isLoaded[1])
	{
		Fail("Reading Data Stream requires at least the table to be read first");
	}

	// Read entire file as contiguous array
	if (stream.seekg(0, std::ios_base::end))
	{
		_soundBuffer.resize(stream.tellg());
		stream.seekg(0);
		stream.read(reinterpret_cast<char*>(_soundBuffer.data()), _soundBuffer.size());
	}

	for (const auto& sample : _table)
	{
		if (sample.offset > _soundBuffer.size())
		{
			Fail("Sound sample offset points to beyond file");
		}
		if (sample.offset + sample.size > _soundBuffer.size())
		{
			Fail("Sound sample size exceeds LHAudioWaveData size");
		}
#if __cplusplus > 201703L
		_sounds.emplace_back(_soundBuffer.data() + sample.relativeOffset, sample.size);
#else
		_sounds.emplace_back(_soundBuffer, sample.offset, sample.size);
#endif
	}

	_isLoaded[2] = true;
}
