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

#pragma once

#include <stdint.h>
#include <string>

namespace OpenBlack
{
class Bitmap16B
{
  public:
	Bitmap16B() = delete;

	Bitmap16B(unsigned int width, unsigned int height);
	Bitmap16B(const void* data, size_t size);
	~Bitmap16B();

	unsigned int Width() const;
	unsigned int Height() const;
	const uint16_t* Data() const;

  private:
	unsigned int _width;
	unsigned int _height;
	uint16_t* _data;

  public:
	static Bitmap16B* LoadFromFile(const std::string& file);
};

} // namespace OpenBlack
