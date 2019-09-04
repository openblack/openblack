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

#include <Common/MemoryStream.h>
#include <cstddef>
#include <algorithm>

namespace OpenBlack
{

MemoryStream::MemoryStream(void* data, std::size_t size)
{
	_data     = data;
	_size     = size;
	_position = 0;
}

std::size_t MemoryStream::Position() const
{
	return _position;
}

std::size_t MemoryStream::Size() const
{
	return _size;
}

void MemoryStream::Seek(std::size_t position, SeekMode seek)
{
	switch (seek)
	{
	case SeekMode::Begin:
		_position = position;
		break;
	case SeekMode::Current:
		_position += position;
		break;
	case SeekMode::End:
		_position = _size + position;
		break;
	}
}

void MemoryStream::Read(void* buffer, std::size_t length)
{
	std::copy_n(reinterpret_cast<const uint8_t*>(_data) + _position, length, reinterpret_cast<uint8_t*>(buffer));
	_position += length;
}

} // namespace OpenBlack
