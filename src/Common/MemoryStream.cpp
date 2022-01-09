/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "MemoryStream.h"

#include <algorithm>
#include <cstddef>
#include <cstdint> // uint8_t

namespace openblack
{

MemoryStream::MemoryStream(const void* data, std::size_t size)
{
	_data = data;
	_size = size;
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

} // namespace openblack
