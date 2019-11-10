/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#include <Common/FileStream.h>
#include <cassert>
#include <spdlog/fmt/fmt.h>
#include <sstream>
#include <stdexcept>

namespace openblack
{

FileStream::FileStream(const fs::path& path, FileMode mode)
{
#ifdef _WIN32
	_wfopen_s(&_file, path.wstring().c_str(), L"rb");
#else
	_file = std::fopen(path.c_str(), "rb");
#endif

	if (_file == nullptr)
		throw std::runtime_error(fmt::format("Failed to open file '{}'", path.string()));

	Seek(0, SeekMode::End);
	_fileSize = Position();
	Seek(0, SeekMode::Begin);
}

FileStream::~FileStream()
{
	if (_file != nullptr)
		std::fclose(_file);
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
	case SeekMode::Begin: std::fseek(_file, static_cast<long>(position), SEEK_SET); break;
	case SeekMode::Current: std::fseek(_file, static_cast<long>(position), SEEK_CUR); break;
	case SeekMode::End: std::fseek(_file, static_cast<long>(position), SEEK_END); break;
	}
}

void FileStream::Read(void* buffer, std::size_t length)
{
	// todo: error check
	std::fread(buffer, length, 1, _file);
}

} // namespace openblack
