/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/
/*
 *
 * The layout of a CAM File is as follows:
 *
 * - 12 byte header
 *         ANM File size - 32-bit unsigned int
 *         movement speed - 32-bit unsigned int
 *         point count - 32 bit unsigned int
 *
 * ------------------------ start of point block -----------------------------
 *
 * - list of point count number of points consiting of
 *         position - 3 floats
 *         heading position - 3 floats
 *
 */
#include "CAMFile.h"

#include <cassert>
#include <cstring>

#include <fstream>

using namespace openblack::cam;

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
	std::streampos seekoff(off_type off, std::ios_base::seekdir way, [[maybe_unused]] std::ios_base::openmode which) override
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

	std::streampos seekpos([[maybe_unused]] pos_type pos, [[maybe_unused]] std::ios_base::openmode which) override
	{
		return seekoff(pos - pos_type(static_cast<off_type>(0)), std::ios_base::beg, which);
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
void CAMFile::Fail(const std::string& msg)
{
	throw std::runtime_error("CAM Error: " + msg + "\nFilename: " + _filename.string());
}

CAMFile::CAMFile() = default;
CAMFile::~CAMFile() = default;

void CAMFile::ReadFile(std::istream& stream)
{
	assert(!_isLoaded);

	// Total file size
	std::size_t fsize = 0;
	if (stream.seekg(0, std::ios_base::end))
	{
		fsize = static_cast<std::size_t>(stream.tellg());
		stream.seekg(0);
	}

	if (fsize < sizeof(CAMHeader))
	{
		Fail("File too small to be a valid CAM file.");
	}

	// First 12 bytes
	stream.read(reinterpret_cast<char*>(&_header), sizeof(_header));

	if (_header.pointCount == 0)
	{
		Fail("CAM File contains no points.");
	}

	if (_header.pointCount * sizeof(CAMPoint) > fsize)
	{
		Fail("CAM File too small to be contain all points.");
	}

	_points.resize(_header.pointCount);
	for (auto& point : _points)
	{
		stream.read(reinterpret_cast<char*>(&point), sizeof(point));
	}
}

void CAMFile::WriteFile([[maybe_unused]] std::ostream& stream) const
{
	assert(!_isLoaded);

	stream.write(reinterpret_cast<const char*>(&_header), sizeof(_header));
	for (const auto& point : _points)
	{
		stream.write(reinterpret_cast<const char*>(&point), sizeof(point));
	}
}

void CAMFile::Open(const std::filesystem::path& filepath)
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

void CAMFile::Open(const std::vector<uint8_t>& buffer)
{
	assert(!_isLoaded);

	imemstream stream(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(buffer[0]));

	// File name set to "buffer" when file is load from a buffer
	// Impact code using CAMFile::GetFilename method
	_filename = std::filesystem::path("buffer");

	ReadFile(stream);
}

void CAMFile::Write(const std::filesystem::path& filepath)
{
	assert(!_isLoaded);

	_filename = filepath;

	std::ofstream stream(_filename, std::ios::binary);

	if (!stream.is_open())
	{
		Fail("Could not open file.");
	}

	_header.pointCount = static_cast<uint32_t>(_points.size());
	_header.size = static_cast<uint32_t>(sizeof(_header) + sizeof(_points[0]) * _header.pointCount);
	WriteFile(stream);
}
