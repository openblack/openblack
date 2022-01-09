/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Enums.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <string>
#include <vector>

namespace openblack::ecs::components
{

struct Stream
{
	using Id = int;

	class Node
	{
	public:
		glm::vec3 position;
		std::vector<Node> edges;

		Node(const glm::vec3& position, const std::vector<Node>& nodes)
		    : position(position)
		{
			if (nodes.empty())
			{
				return;
			}

			auto element =
			    std::min_element(std::cbegin(nodes), std::cend(nodes), [&position](const auto& first, const auto& second) {
				    auto firstDistance = glm::distance(position, first.position);
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

	Stream::Id id;
	std::vector<Node> nodes;
};

} // namespace openblack::ecs::components
