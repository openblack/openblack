/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GLWFile.h"

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <fstream>
#include <ios>
#include <iosfwd>
#include <spanstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <vector>

using namespace openblack::glw;

void GLWFile::Fail(const std::string& msg)
{
	throw std::runtime_error("GLW Error: " + msg + "\nFilename: " + _filename.string());
}

void GLWFile::ReadFile(std::istream& stream)
{
	assert(!_isLoaded);

	std::size_t fsize = 0;
	if (stream.seekg(0, std::ios_base::end))
	{
		fsize = static_cast<std::size_t>(stream.tellg());
		stream.seekg(0);
	}

	while ((fsize - stream.tellg()) > sizeof(Glow))
	{
		Glow glow;
		stream.read(reinterpret_cast<char*>(&glow), sizeof(glow));
		_glows.emplace_back(glow);
	}

	uint32_t glowCount;
	stream.read(reinterpret_cast<char*>(&glowCount), sizeof(glowCount));

	if (glowCount != _glows.size())
	{
		Fail("Less glow items than expected");
	}
}

void GLWFile::WriteFile(std::ostream& stream) const
{
	for (auto glow : _glows)
	{
		stream.write(reinterpret_cast<char*>(&glow), sizeof(glow));
	}
	auto glowCount = static_cast<uint32_t>(_glows.size());
	stream.write(reinterpret_cast<char*>(&glowCount), sizeof(glowCount));
}

GLWFile::GLWFile() = default;
GLWFile::~GLWFile() = default;

void GLWFile::Open(const std::filesystem::path& filepath)
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

void GLWFile::Open(const std::span<char>& span)
{
	assert(!_isLoaded);

	auto stream = std::spanstream(span);

	// File name set to "buffer" when file is load from a buffer
	// Impact code using GLWFile::GetFilename method
	_filename = std::filesystem::path("buffer");

	ReadFile(stream);
}

void GLWFile::Write(const std::filesystem::path& filepath)
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
