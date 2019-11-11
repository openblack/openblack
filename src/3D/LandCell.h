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

#include <cstdint>

namespace openblack
{
#pragma pack(push, 1)
class LandCell
{
public:
	LandCell() {}

	[[nodiscard]] inline uint8_t Light() const { return luminosity; }
	[[nodiscard]] inline uint8_t Altitude() const { return altitude; }
	[[nodiscard]] inline bool Split() const { return properties & 0x80U; }
	[[nodiscard]] inline bool HasWater() const { return properties & 0x10U; }
	[[nodiscard]] inline bool Coastline() const { return properties & 0x20U; }
	[[nodiscard]] inline bool FullWater() const { return properties & 0x40U; } // TODO ? unsure (could mean deep water)
	[[nodiscard]] inline uint8_t Country() const { return properties & 0x0FU; }

	// TODO: this is temporary way for drawing landscape, should be moved to the renderer
	[[nodiscard]] inline float Alpha() const
	{
		if (FullWater() || HasWater())
			return 0.0f;
		if (Coastline())
			return 0.5f;
		return 1.0f;
	}

private:
	uint8_t color[3] {0, 0, 0};
	uint8_t luminosity {0};
	uint8_t altitude {0};
	uint8_t savecolor;

	uint8_t properties {0x40};
	// Sound properties: coastal sound, land sound, sea sound, freshwater sound
	uint8_t flags2;
};
#pragma pack(pop)
} // namespace openblack
