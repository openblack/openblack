/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
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

MemoryStream::MemoryStream(void* data, std::size_t size)
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
	// Ensure it doesn't read beyond buffer length
	if (length > _size - (_position + 1))
		length = (_size - (_position + 1));

	std::copy_n(reinterpret_cast<uint8_t*>(_data) + _position, length, reinterpret_cast<uint8_t*>(buffer));
	_position += length;
}

void MemoryStream::Write(const void* buffer, std::size_t length)
{
	std::copy_n(reinterpret_cast<const uint8_t*>(buffer), length, reinterpret_cast<uint8_t*>(_data));
}

} // namespace openblack
