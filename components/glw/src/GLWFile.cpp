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
#include <cstddef>
#include <cstdint>

#include <fstream>
#include <ios>
#include <iosfwd>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <utility>
#include <vector>

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
		return seekoff(pos - static_cast<off_type>(0), std::ios_base::beg, which);
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

GLWResult GLWFile::Open(const std::vector<uint8_t>& buffer) noexcept
{
	assert(!_isLoaded);

	auto stream = imemstream(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(buffer[0]));

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
