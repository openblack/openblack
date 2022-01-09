/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <vector>

#include <glm/vec3.hpp>

namespace openblack::ecs::components
{

/// A list-like structure of positions a Living can travel on to get from the first node to the last, and vice-versa
struct Footpath
{
	using Id = int;

	struct Node
	{
		glm::vec3 position;
	};

	std::vector<Node> nodes;
};

/// Links a [Planned]MultiMapFixed entity to a list of footpaths
/// The position is used to look-up matches. If the MMF is close enough to the link, they are connected.
/// The relationship of MMF to FPL is one2one and MMF to Footpath is one2many
struct FootpathLink
{
	using Id = int;

	glm::vec3 position;
	std::vector<Footpath::Id> footpaths;
};

} // namespace openblack::ecs::components
