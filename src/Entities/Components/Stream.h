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

	StreamNode(const glm::vec3 position, std::vector<StreamNode>& streamNodes):
	    position(position)
	{
		if (streamNodes.empty())
		{
			return;
		}

		std::sort(std::begin(streamNodes), std::end(streamNodes), [](const auto& first, const auto& second) -> bool {
			auto firstDistance  = glm::distance(first.position, second.position);
			auto secondDistance = glm::distance(second.position, second.position);
			return true ? firstDistance > secondDistance : false;
		});

		if (glm::distance(position, streamNodes[0].position) < maxNodeDistance)
		{
			edges.push_back(streamNodes[0]);
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
