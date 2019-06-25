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

#pragma once

#include <filesystem>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <cstdio>
#endif

namespace OpenBlack
{

enum class FileMode
{
	Read,
	Write
};

enum class FileSeekMode
{
	Begin,
	Current,
	End
};

class File
{
  public:
	File(const std::filesystem::path& path, FileMode mode);
	~File();

	void Close();

	const size_t Read(uint8_t* buffer, size_t size);
	const size_t Write(const uint8_t* buffer, size_t size);

	template <typename T>
	const T Read();

	void Seek(size_t position, FileSeekMode mode);
	const size_t Position() const;
	const size_t Size() const;

	const std::filesystem::path GetPath() { return _path; }

  protected:
#ifdef _WIN32
	HANDLE _file;
#else
	FILE* _file;
#endif
	std::filesystem::path _path;
};

} // namespace OpenBlack
