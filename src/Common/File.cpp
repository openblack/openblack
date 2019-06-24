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

File::File(const std::filesystem::path& path, FileMode mode):
    _file(nullptr),
    _path(path)
{
	_file = fopen(
	    path.u8string().c_str(),
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

size_t File::Read(uint8_t* buffer, size_t size)
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

} // namespace OpenBlack
