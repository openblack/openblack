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

#include <stdint.h>
#include <string>

namespace openblack
{
class Bitmap16B
{
public:
	Bitmap16B() = delete;

	Bitmap16B(const void* data, size_t size);
	~Bitmap16B();

	unsigned int Width() const { return _width; }
	unsigned int Height() const { return _height; }
	uint16_t* Data() { return _data; }
	size_t Size() const { return _size; };

private:
	unsigned int _width;
	unsigned int _height;
	uint16_t* _data;
	size_t _size;

public:
	static Bitmap16B* LoadFromFile(const std::string& file);
};

} // namespace openblack
