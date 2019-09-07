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

namespace openblack
{

enum class SeekMode
{
	Begin,
	Current,
	End
};

class IStream
{
  public:
	// virtual ~IStream() {}

	virtual std::size_t Position() const = 0;
	virtual std::size_t Size() const = 0;
	virtual void Seek(std::size_t position, SeekMode seek) = 0;

	virtual void Read(void* buffer, std::size_t length) = 0;
	// virtual void Write(const void* buffer, std::size_t length) = 0;

	template <typename T>
	void Read(T* value) { Read(value, sizeof(T)); }

	// template <typename T>
	// void Write(const T* value) { Read(value, sizeof(T)); }

	template <typename T>
	T ReadValue()
	{
		T value;
		Read(&value);
		return value;
	}
};

} // namespace openblack
