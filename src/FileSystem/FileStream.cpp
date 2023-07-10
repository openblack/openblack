/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "FileStream.h"

#include <cassert>

#include <stdexcept>

#include <spdlog/fmt/fmt.h>

using namespace openblack::filesystem;

FileStream::FileStream(const std::filesystem::path& path, FileMode mode)
{
	std::wstring recognisedMode;

	switch (mode)
	{
	case FileMode::Read:
		recognisedMode = L"rb";
		break;
	case FileMode::Write:
		recognisedMode = L"wb";
		break;
	case FileMode::Append:
		recognisedMode = L"ab";
		break;
	}

#ifdef _WIN32
	_wfopen_s(&_file, path.wstring().c_str(), recognisedMode.c_str());
#else
	_file = std::fopen(path.c_str(), "rb");
#endif

	if (_file == nullptr)
	{
		throw std::runtime_error(fmt::format("Failed to open file '{}'", path.string()));
	}

	Seek(0, SeekMode::End);
	_fileSize = Position();
	Seek(0, SeekMode::Begin);
}

FileStream::~FileStream()
{
	if (_file != nullptr)
	{
		std::fclose(_file);
	}
}

std::size_t FileStream::Position() const
{
	return static_cast<std::size_t>(std::ftell(_file));
}

std::size_t FileStream::Size() const
{
	return _fileSize;
}

void FileStream::Seek(std::size_t position, SeekMode seek)
{
	switch (seek)
	{
	case SeekMode::Begin:
		// NOLINTNEXTLINE(google-runtime-int): required type for fseek
		std::fseek(_file, static_cast<long>(position), SEEK_SET);
		break;
	case SeekMode::Current:
		// NOLINTNEXTLINE(google-runtime-int): required type for fseek
		std::fseek(_file, static_cast<long>(position), SEEK_CUR);
		break;
	case SeekMode::End:
		// NOLINTNEXTLINE(google-runtime-int): required type for fseek
		std::fseek(_file, static_cast<long>(position), SEEK_END);
		break;
	}
}

Stream& FileStream::Read(void* buffer, std::size_t length)
{
	size_t size = std::fread(buffer, 1, length, _file);
	if (size != length)
	{
		throw std::runtime_error(fmt::format("Error while reading file"));
	}
	return *this;
}

Stream& FileStream::Write(const void* buffer, std::size_t length)
{
	size_t size = std::fwrite(buffer, 1, length, _file);
	if (size != length)
	{
		throw std::runtime_error(fmt::format("Error while writing file"));
	}
	return *this;
}
