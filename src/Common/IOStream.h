/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <cstddef>

namespace openblack
{

enum class SeekMode
{
	Begin,
	Current,
	End
};

class IOStream
{
public:
	// virtual ~IStream() {}

	[[nodiscard]] virtual std::size_t Position() const = 0;
	[[nodiscard]] virtual std::size_t Size() const = 0;
	virtual void Seek(std::size_t position, SeekMode seek) = 0;

	virtual void Read(void* buffer, std::size_t length) = 0;
	virtual void Write(const void* buffer, std::size_t length) = 0;

	template <typename T>
	void Read(T* value)
	{
		Read(value, sizeof(T));
	}

	template <typename T>
	T ReadValue()
	{
		T value;
		Read(&value);
		return value;
	}

	template <typename T>
	void WriteValue(T value, unsigned size = sizeof(T))
	{
		for (; size; --size, value >>= 8)
		{
			auto val = static_cast<char>(value & 0xFF);
			Write(&val, 1);
		}
	}
};

} // namespace openblack
