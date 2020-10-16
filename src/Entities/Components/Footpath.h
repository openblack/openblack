/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <glm/vec3.hpp>
#include <vector>

namespace openblack
{

struct FootpathNode
{
	glm::vec3 position;
};

struct Footpath
{
	int id;
	std::vector<FootpathNode> nodes;
};

} // namespace openblack
