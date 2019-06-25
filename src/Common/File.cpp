/* OpenBlack - A reimplementation of Lionhead's Black & White.
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#include <Common/File.h>
#include <cassert>
#include <sstream>
#include <stdexcept>

namespace OpenBlack
{

#ifdef _WIN32
File::File(const std::filesystem::path& path, FileMode mode):
    _file(INVALID_HANDLE_VALUE),
    _path(path)
{
	_file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (_file == INVALID_HANDLE_VALUE)
		throw std::runtime_error("CreateFileW failed");
}

File::~File()
{
	if (_file != INVALID_HANDLE_VALUE)
		CloseHandle(_file);
}

void File::Close()
{
	if (_file != INVALID_HANDLE_VALUE)
		CloseHandle(_file);
}

const size_t File::Read(uint8_t* buffer, size_t size)
{
	assert(_file != INVALID_HANDLE_VALUE);

	DWORD read;
	if (!ReadFile(mHandle, data, size, &read, NULL))
		throw std::runtime_error("A read operation on a file failed.");

	return read;
}

void File::Seek(size_t position, FileSeekMode mode)
{
	assert(_file != INVALID_HANDLE_VALUE);

	int origin;
	switch (mode)
	{
	case FileSeekMode::Begin:
		origin = FILE_BEGIN;
		break;
	case FileSeekMode::Current:
		origin = FILE_CURRENT;
		break;
	case FileSeekMode::End:
		origin = FILE_END;
		break;
	default:
		throw std::runtime_error("invalid seek mode");
	}

	if (SetFilePointer(_file, position, NULL, origin) == INVALID_SET_FILE_POINTER)
		if (GetLastError() != NO_ERROR)
			throw std::runtime_error("A seek operation on a file failed.");
}

const size_t File::Position() const
{
	assert(_file != INVALID_HANDLE_VALUE);

	DWORD value = SetFilePointer(_file, 0, NULL, SEEK_CUR);

	if (value == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		throw std::runtime_error("A query operation on a file failed.");

	return static_cast<size_t>(value);
}

const size_t File::Size() const
{
	assert(_file != INVALID_HANDLE_VALUE);

	LARGE_INTEGER size;
	DWORD high;
	DWORD low = GetFileSize(_file, &high);
	if (low != 0xFFFFFFFF || GetLastError() == NO_ERROR)
	{
		size.LowPart  = low;
		size.HighPart = high;
		return size.QuadPart;
	}
	return 0;
}

#else
File::File(const std::filesystem::path& path, FileMode mode):
    _file(nullptr),
    _path(path)
{
	_file = fopen(
	    path.c_str(),
	    mode == FileMode::Read ? "rb" : "wb");

	// todo: throw a real exception with errno
	if (_file == nullptr)
		throw std::runtime_error("fopen failed");
}

File::~File()
{
	if (_file != nullptr)
		fclose(_file);
}

void File::Close()
{
	if (_file != nullptr)
		fclose(_file);
}

const size_t File::Read(uint8_t* buffer, size_t size)
{
	assert(_file != nullptr);

	// todo: throw errors properly
	return fread(buffer, 1, size, _file);
}

void File::Seek(size_t position, FileSeekMode mode)
{
	assert(_file != nullptr);

	int origin;
	switch (mode)
	{
	case FileSeekMode::Begin:
		origin = SEEK_SET;
		break;
	case FileSeekMode::Current:
		origin = SEEK_CUR;
		break;
	case FileSeekMode::End:
		origin = SEEK_END;
		break;
	default:
		throw std::runtime_error("invalid seek mode");
	}

	// todo: erro rcheck
	fseek(_file, position, origin);
}

// todo: File::Position
// todo: File::Size
#endif

} // namespace OpenBlack
