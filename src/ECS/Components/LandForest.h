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

namespace openblack::ecs::components
{

/// One of the land's forests: a place its trees are gathered about (the entity's Transform), the trees being those whose
/// ForestMember has its number
struct LandForest
{
	/// Its number, which the land's script gives its trees
	uint32_t id {0};
	/// The big forest it is the forest of, if any
	entt::entity bigForest {entt::null};
	/// A town's forest of the lone trees about it, which never grows trees of its own
	bool scenic {false};
	/// The order the forests were made in: the land's forests are met newest first
	uint32_t made {0};
};

/// The forests near a town, which its people go to for wood
struct TownForests
{
	std::vector<entt::entity> forests;
};

} // namespace openblack::ecs::components
