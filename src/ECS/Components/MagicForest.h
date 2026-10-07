/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack::ecs::components
{

/// The forest a forest miracle planted, standing where its first tree went: the trees it keeps growing while the miracle
/// lasts, which wither away once it has gone
struct MagicForest
{
	/// The miracle keeping it, none once it has gone
	entt::entity spell {entt::null};
	PlayerNames player {PlayerNames::NEUTRAL};
	/// Where the miracle was cast, the middle of its spiral
	glm::vec3 centre {0.0f};
	std::vector<entt::entity> trees;
};

/// On a tree a forest miracle planted: its forest, and what its wood is worth against an ordinary tree's
struct MagicTree
{
	entt::entity forest {entt::null};
	float woodMultiplier {1.0f};
	/// How impressive it is to the people who come to look at it
	float impressiveValue {0.0f};
};

} // namespace openblack::ecs::components
