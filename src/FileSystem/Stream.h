/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>

namespace openblack::filesystem
{

class Stream
{
public:
	enum class Mode
	{
		Read,
		Write,
		Append
	};

	enum class SeekMode
	{
		Begin,
		Current,
		End
	};

	virtual ~Stream() = default;

	[[nodiscard]] virtual std::size_t Position() const = 0;
	[[nodiscard]] virtual std::size_t Size() const = 0;
	virtual void Seek(std::size_t position, SeekMode seek) = 0;

	virtual Stream& Read(uint8_t* buffer, std::size_t length) = 0;
	virtual Stream& Write(const uint8_t* buffer, std::size_t length) = 0;

	template <typename T>
	Stream& Read(T* value)
	{
		return Read(reinterpret_cast<uint8_t*>(value), sizeof(T));
	}

	template <typename T>
	Stream& Write(const T* value)
	{
		return Write(reinterpret_cast<const uint8_t*>(value), sizeof(T));
	}

	template <typename T>
	T ReadValue()
	{
		T value;
		Read(&value);
		return value;
	}

	template <typename T>
	Stream& WriteValue(const T& value)
	{
		return Write(&value, sizeof(T));
	}
};

} // namespace openblack::filesystem
