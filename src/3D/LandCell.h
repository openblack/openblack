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
#ifndef OPENBLACK_LANDCELL_H
#define OPENBLACK_LANDCELL_H

#include <Common/Types.h>

namespace OpenBlack
{
#pragma pack(push, 1)
class LandCell
{
  public:
	LandCell() : altitude(0), properties(0x40) {}

	inline uint8_t Light() { return color.a; }
	inline uint8_t Altitude() const { return altitude; }
	inline bool Split() { return properties & 0x80; }
	inline bool HasWater() { return properties & 0x10; }
	inline bool Coastline() { return properties & 0x20; }
	inline bool FullWater() { return properties & 0x40; }
	inline uint8_t Country() { return properties & 0x0F; }
	inline float Alpha()
	{
		if (properties & 0x40)
			return 0.0f;
		if (properties & 0x10)
			return 0.0f;
		if (properties & 0x20)
			return 0.5f;

		return 1.0f;
	}

  private:
	rgba_t color;

	uint8_t altitude;
	uint8_t savecolor;

	uint8_t properties;
	// Sound properties: coastal sound, land sound, sea sound, freshwater sound
	uint8_t flags2;
};
#pragma pack(pop)
} // namespace OpenBlack

#endif
