/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "File.h"

namespace openblack
{
File::File(const std::filesystem::path& path, Mode mode)
{
	const char* mode_string;

	if (mode == Mode::Read)
		mode_string = "rb";
	else if (mode == Mode::Write)
		mode_string = "wb";

	_file = std::fopen(path.string().c_str(), "rb");

	if (_file == nullptr) {
		switch (errno) {
		case ENOENT:
			throw std::runtime_error("File not found: " + path.string());
		default:
			throw std::runtime_error("Can not open file: " + path.string());
		}
	}

	_fileName = path.filename().string();
}

File::~File()
{
	std::fclose(_file);
}

bool File::Read(std::uint8_t* dest, std::size_t bytesToRead)
{
	return std::fread(dest, 1, bytesToRead, _file) == bytesToRead;
}

bool File::Write(std::uint8_t* source, std::size_t bytesToWrite)
{
	return std::fwrite(source, 1, bytesToWrite, _file) == bytesToWrite;
}

bool File::Flush()
{
	return std::fflush(_file) == 0;
}

bool File::Seek(std::size_t position, SeekOrigin origin)
{
	int origin_int = 0;
	switch (origin)
	{
	case SeekOrigin::Begin:
		origin_int = SEEK_SET;
		break;
	case SeekOrigin::Current:
		origin_int = SEEK_CUR;
		break;
	case SeekOrigin::End:
		origin_int = SEEK_END;
		break;
	}

	return std::fseek(_file, static_cast<long>(position), origin_int) == 0;
}

[[nodiscard]] std::size_t File::GetPosition()
{
	return static_cast<std::size_t>(std::ftell(_file));
}

std::size_t IFile::GetSize()
{
	auto cur = GetPosition();
	Seek(0, SeekOrigin::End);
	auto size = GetPosition();
	Seek(cur, SeekOrigin::Begin);

	return size;
}

}
