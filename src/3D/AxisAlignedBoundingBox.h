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

#include <glm/vec3.hpp>

namespace openblack
{

struct AxisAlignedBoundingBox
{
	glm::vec3 minima;
	glm::vec3 maxima;

	[[nodiscard]] inline glm::vec3 center() const { return (maxima + minima) * 0.5f; }
	[[nodiscard]] inline glm::vec3 size() const { return maxima - minima; }
};

} // namespace openblack
