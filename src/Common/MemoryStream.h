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

#include <Common/IStream.h>

namespace openblack
{

class MemoryStream: public IStream
{
public:
	MemoryStream(void* data, std::size_t size);

	std::size_t Position() const override;
	std::size_t Size() const override;
	void Seek(std::size_t position, SeekMode seek) override;

	void Read(void* buffer, std::size_t length) override;

protected:
	void* _data;
	std::size_t _size;
	std::size_t _position;
};

} // namespace openblack
