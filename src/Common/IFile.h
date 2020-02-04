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

#pragma once

#include <cstddef>
#include <cstdint>

namespace openblack
{

enum class SeekOrigin
{
	Begin,
	Current,
	End
};

/**
 * File interface.
 */
class IFile
{
public:
	/** Destructor, for closing the file. */
	virtual ~IFile() = default;

	/**
	 * Read bytes from the file into the destination.
	 *
	 * @param dest        Destinatation for the bytes read.
	 * @param bytesToRead Number of bytes to read into dest.
	 * @return            True if successful.
	 */
	virtual bool Read(std::uint8_t* dest, std::size_t bytesToRead) = 0;

	/**
	 * Write bytes to the file from source.
	 *
	 * @param source      Source buffer to read from.
	 * @param bytesToRead Number of bytes to write to the file from source.
	 * @return            True if successful.
	 */
	virtual bool Write(std::uint8_t* source, std::size_t bytesToWrite) = 0;

	/**
	 * Flushes file to disk.
	 */
	virtual bool Flush() = 0;

	/**
	 * Set the current read or write position to as specified.
	 *
	 * @param position New read or write position.
	 * @param origin   From where to base the new position from.
	 * @return True if successful.
	 */
	virtual bool Seek(std::size_t position, SeekOrigin origin) = 0;

	/**
	 * Get the current read or write position.
	 */
	[[nodiscard]] virtual std::size_t GetPosition() = 0;

public:
	// Utility functions that have default implementations based on virtual operations.

	/** Total size of the file. */
	[[nodiscard]] virtual std::size_t GetSize();

public:
	// Templated type readers

	template <typename T>
	bool Read(T* data, std::size_t length = 1)
	{
		return Read(reinterpret_cast<uint8_t*>(data), sizeof(T)* length);
	}

	template <typename T>
	T Read()
	{
		T value;
		Read<T>(&value);
		return value;
	}

protected:
	constexpr IFile() = default;
};

} // namespace openblack
