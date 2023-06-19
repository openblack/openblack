/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "MemoryStream.h"

#include <cstddef>
#include <cstdint>

#include <algorithm>

using namespace openblack::filesystem;

MemoryStream::MemoryStream(std::vector<uint8_t>&& data)
    : _data(data)
    , _position(0)
{
}

std::size_t MemoryStream::Position() const
{
	return _position;
}

std::size_t MemoryStream::Size() const
{
	return _data.size();
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
		_position = _data.size() + position;
		break;
	}
}

Stream& MemoryStream::Read(uint8_t* buffer, std::size_t length)
{
	std::copy_n(_data.data() + _position, length, buffer);
	_position += length;
	return *this;
}

Stream& MemoryStream::Write(const uint8_t* buffer, std::size_t length)
{
	std::copy_n(buffer, length, _data.data() + _position);
	_position += length;
	return *this;
}

std::string MemoryStream::GetLine()
{
	auto it = std::find(_data.begin() + _position, _data.end(), '\n');

	std::string line;
	if (it != _data.end())
	{
		line.assign(_data.begin() + _position, it);
		_position = std::distance(_data.begin(), it) + 1; // move the cursor past the '\n'
	}
	else
	{
		line.assign(_data.begin() + _position, _data.end());
		_position = _data.size(); // move the cursor to the end
	}

	return line;
}

bool MemoryStream::IsEndOfFile() const
{
	return Position() >= Size();
}
