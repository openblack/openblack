/*******************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************
 *
 * The layout of a ANM File is as follows:
 *
 * - 84 byte header
 *         animation name, containing 32 chars
 *         1 unknown 32-bit int, likely a short // TODO
 *         5 unknown floats // TODO
 *         frame count
 *         1 unknown 32-bit int, likely a bool // TODO
 *         animation duration
 *         2 unknown 32-bit ints, likely bools // TODO
 *         keyframe start offset - offset of keyframe offset block (see below)
 *         1 unknown 32-bit int, likely a short // TODO
 *
 * ------------------------ start of keyframe offsets block --------------------
 *
 * - 4 bytes * frame count, each record contains:
 *         offset into keyframe pointer block (see below)
 *
 * ------------------------ start of keyframe pointer block --------------------
 *
 * - 4 bytes offset containing:
 *         offset into bone offset block (see below)
 *
 * ------------------------ start of bone offset block -------------------------
 *
 * - 4 bytes offset containing:
 *         offset into bone block (see below)
 *
 * ------------------------ start of bone block --------------------------------
 *
 * - 8 byte header containing:
 *         number of bones in the block
 *         timestamp of animation
 * - Bone matrix list of length of number of bones each record containing:
 *         a 4x3 matrix
 *
 */

#include "ANMFile.h"

#include <cassert>
#include <cstring>

#include <fstream>
#include <vector>

using namespace openblack::anm;

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
void ANMFile::Fail(const std::string& msg)
{
	throw std::runtime_error("ANM Error: " + msg + "\nFilename: " + _filename.string());
}

ANMFile::ANMFile()
    : _isLoaded(false)
{
}

void ANMFile::ReadFile(std::istream& stream)
{
	assert(!_isLoaded);

	// Total file size
	std::size_t fsize = 0;
	if (stream.seekg(0, std::ios_base::end))
	{
		fsize = static_cast<std::size_t>(stream.tellg());
		stream.seekg(0);
	}

	if (fsize < sizeof(ANMHeader))
	{
		Fail("File too small to be a valid ANM file.");
	}

	// First 84 bytes
	stream.read(reinterpret_cast<char*>(&_header), sizeof(ANMHeader));

	_keyframes.resize(_header.frame_count);
	for (uint32_t i = 0; i < _header.frame_count; ++i)
	{
		stream.seekg(_header.frames_base + i * sizeof(uint32_t));

		// In Keyframe offset block
		uint32_t offset;
		stream.read(reinterpret_cast<char*>(&offset), sizeof(offset));

		// Keyframe pointer
		stream.seekg(offset);
		stream.read(reinterpret_cast<char*>(&offset), sizeof(offset));

		// Bone offset block
		stream.seekg(offset);
		stream.read(reinterpret_cast<char*>(&offset), sizeof(offset));

		// Bone block
		stream.seekg(offset);
		uint32_t bone_count;
		stream.read(reinterpret_cast<char*>(&bone_count), sizeof(bone_count));

		stream.read(reinterpret_cast<char*>(&_keyframes[i].time), sizeof(_keyframes[i].time));

		_keyframes[i].bones.resize(bone_count);
		stream.read(reinterpret_cast<char*>(_keyframes[i].bones.data()),
		            _keyframes[i].bones.size() * sizeof(_keyframes[i].bones[0]));
	}

	_isLoaded = true;
}

void ANMFile::WriteFile([[maybe_unused]] std::ostream& stream) const
{
	assert(!_isLoaded);
	stream.write(reinterpret_cast<const char*>(&_header), sizeof(_header));
}

void ANMFile::Open(const std::filesystem::path& filepath)
{
	assert(!_isLoaded);

	_filename = filepath;

	std::ifstream stream(_filename, std::ios::binary);

	if (!stream.is_open())
	{
		Fail("Could not open file.");
	}

	ReadFile(stream);
}

void ANMFile::Open(const std::vector<uint8_t>& buffer)
{
	assert(!_isLoaded);

	imemstream stream(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(buffer[0]));

	// File name set to "buffer" when file is load from a buffer
	// Impact code using ANMFile::GetFilename method
	_filename = std::filesystem::path("buffer");

	ReadFile(stream);
}

void ANMFile::Write(const std::filesystem::path& filepath)
{
	assert(!_isLoaded);

	_filename = filepath;

	std::ofstream stream(_filename, std::ios::binary);

	if (!stream.is_open())
	{
		Fail("Could not open file.");
	}

	WriteFile(stream);
}
