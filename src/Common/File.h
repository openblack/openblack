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

#include <experimental/filesystem>
#include <string>
#include <cstdio>
#include <cassert>
#include <unordered_map>

namespace fs = std::experimental::filesystem;

namespace OpenBlack
{

enum class FileMode
{
	Read,
	Write
};

enum class FileSeekMode
{
	Begin   = SEEK_SET,
	Current = SEEK_CUR,
	End     = SEEK_END
};

class File
{
  public:
	File();
	File(const fs::path& filename, FileMode mode);
	~File();

	void Open(const fs::path& filename, FileMode mode);
	void Close();

	template <typename T>
	std::size_t Read(T* data, std::size_t length) const
	{
		assert(_file != nullptr);
		static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
		return std::fread(data, sizeof(T), length, _file);
	}

	template <typename T>
	std::size_t ReadBytes(T* data, std::size_t length) const
	{
		assert(_file != nullptr);
		static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
		return Read(reinterpret_cast<uint8_t*>(data), length);
	}

	void Seek(std::size_t position, FileSeekMode mode) const;
	std::size_t Position() const;
	std::size_t Size() const;
	void Flush();
  protected:
	FILE* _file;
};

class LHSegmentedFile
{
	struct LHSegment
	{
		uint32_t offset;
		std::size_t size;
	};

  public:
	LHSegmentedFile(const fs::path& filename, FileMode mode);

  protected:
	std::unique_ptr<File> _file;
	std::unordered_map<std::string, LHSegment> _segments;
};

} // namespace OpenBlack
