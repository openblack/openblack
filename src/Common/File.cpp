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

File::File() {}

File::File(const std::filesystem::path& path, FileMode mode):
    _file(nullptr)
{
	Open(path, mode);
}

File::~File()
{
	Close();
}

void File::Open(const std::filesystem::path& path, FileMode mode)
{
	Close();

#ifdef _WIN32
	_wfopen_s(&_file, path.wstring().c_str(), L"rb");
#else
	_file = std::fopen(path.c_str(), "rb");
#endif

	if (_file == nullptr)
		throw std::runtime_error("fopen failed");
}

void File::Close()
{
	if (_file != nullptr)
		std::fclose(_file);

	_file = nullptr;
}

void File::Seek(size_t position, FileSeekMode mode) const
{
	assert(_file != nullptr);
	std::fseek(_file, static_cast<long>(position), static_cast<int>(mode));
}

size_t File::Position() const
{
	assert(_file != nullptr);
	return static_cast<size_t>(std::ftell(_file));
}

size_t File::Size() const
{
	assert(_file != nullptr);

	size_t origPos = Position();
	Seek(0, FileSeekMode::End);

	size_t size = Position();
	Seek(origPos, FileSeekMode::Begin);

	return size;
}

void File::Flush()
{
	assert(_file != nullptr);
	std::fflush(_file);
}

} // namespace OpenBlack
