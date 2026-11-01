/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GLWFile.h"

#include <cassert>
#include <cstdint>

#include <fstream>
#include <ios>
#include <spanstream>
#include <streambuf>
#include <string>
#include <utility>
#include <vector>

using namespace openblack::glw;

std::string_view openblack::glw::ResultToStr(GLWResult result)
{
	switch (result)
	{
	case GLWResult::Success:
		return "Success";
	case GLWResult::ErrCantOpen:
		return "Could not open file.";
	case GLWResult::ErrItemCountMismatch:
		return "Less glow items than expected.";
	}
	std::unreachable();
}

GLWResult GLWFile::ReadFile(std::istream& stream) noexcept
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
		return GLWResult::ErrItemCountMismatch;
	}

	return GLWResult::Success;
}

GLWResult GLWFile::WriteFile(std::ostream& stream) const noexcept
{
	for (auto glow : _glows)
	{
		stream.write(reinterpret_cast<char*>(&glow), sizeof(glow));
	}
	auto glowCount = static_cast<uint32_t>(_glows.size());
	stream.write(reinterpret_cast<char*>(&glowCount), sizeof(glowCount));

	return GLWResult::Success;
}

GLWFile::GLWFile() noexcept = default;
GLWFile::~GLWFile() noexcept = default;

GLWResult GLWFile::Open(const std::filesystem::path& filepath) noexcept
{
	assert(!_isLoaded);

	std::ifstream stream(filepath, std::ios::binary);

	if (!stream.is_open())
	{
		return GLWResult::ErrCantOpen;
	}

	return ReadFile(stream);
}

GLWResult GLWFile::Open(const std::span<char>& span) noexcept
{
	assert(!_isLoaded);

	auto stream = std::spanstream(span);

	return ReadFile(stream);
}

GLWResult GLWFile::Write(const std::filesystem::path& filepath) noexcept
{
	assert(!_isLoaded);
	std::ofstream stream(filepath, std::ios::binary);

	if (!stream.is_open())
	{
		return GLWResult::ErrCantOpen;
	}

	return WriteFile(stream);
}
