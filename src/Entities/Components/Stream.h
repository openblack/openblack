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

#include <Enums.h>
#include <algorithm>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace openblack
{

class StreamNode
{
  public:
	glm::vec3 position;
	std::vector<StreamNode> edges;

	StreamNode(const glm::vec3 position, const std::vector<StreamNode>& streamNodes):
	    position(position)
	{
		if (streamNodes.empty())
		{
			return;
		}

		auto element = std::min_element(std::cbegin(streamNodes), std::cend(streamNodes), [&position](const auto& first, const auto& second) {
			auto firstDistance  = glm::distance(position, first.position);
			auto secondDistance = glm::distance(position, second.position);
			return firstDistance < secondDistance;
		});

		if (glm::distance(position, element->position) < maxNodeDistance)
		{
			edges.push_back(*element);
		}
	}
  private:
	const static auto maxNodeDistance = 100;
};

struct Stream
{
	int streamId;
	std::vector<StreamNode> streamNodes;
};

} // namespace openblack
