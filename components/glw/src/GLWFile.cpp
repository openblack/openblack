/*******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GLWFile.h"

#include <cassert>

#include <fstream>

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

void GLWFile::Fail(const std::string& msg)
{
	throw std::runtime_error("GLW Error: " + msg + "\nFilename: " + _filename.string());
}

void GLWFile::ReadFile(std::istream& stream)
{
	assert(!_isLoaded);

	// Total file size
	std::size_t fsize = 0;
	if (stream.seekg(0, std::ios_base::end))
	{
		fsize = static_cast<std::size_t>(stream.tellg());
		stream.seekg(0);
	}

	while ((fsize - stream.tellg()) > sizeof(GlowEntry))
	{
		GlowEntry glow;
		stream.read(reinterpret_cast<char*>(&glow), sizeof(glow));
		_glows.emplace_back(glow);
	}

	uint32_t glowCount;
	stream.read(reinterpret_cast<char*>(&glowCount), sizeof(glowCount));

	if (glowCount != _glows.size())
	{
		Fail("Invalid number of glows");
	}
}

void GLWFile::WriteFile(std::ostream& stream) const
{
	for (auto glow : _glows)
	{
		glow.red = 3.0f;
		glow.green = 0;
		glow.blue = 0;
		glow.dirX = 0;
		glow.dirY = 0;
		glow.dirZ = 0;
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
