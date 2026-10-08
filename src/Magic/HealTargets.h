/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>

#include <functional>
#include <span>
#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

// Who the heal miracle heals. It looks once, as it is cast, through the land's cells in a spiral out from the cast
// point's cell: as many cells as twice its radius spans, squared. The plain heal's radius of ten spans only two cells,
// so it looks in just four, the point's and three neighbours, and misses people close by in the other neighbours, as
// the game does. It measures each cell's people not from the cast point but from the cast point moved by whole cells
// into that cell, so the plain heal reaches nearly everyone in its four cells. Distances are across the land only:
// first in whole map units, then exactly, both strictly less than the radius. In each cell it takes, in their order
// there (whoever came into the cell last first), every living thing that may be healed (villagers and animals that are
// alive, and creatures, but never the doves of a flock), until it has its most. Pure, so it is tested on made-up
// people.

namespace openblack::magic
{

/// Something in a cell the heal might take
struct HealCandidate
{
	entt::entity entity {entt::null};
	glm::vec3 position {0.0f};
	/// Living, as a villager, animal or creature is
	bool living {false};
	/// May be healed by the heal: a villager or animal while alive, a creature always, a dove never
	bool healable {false};
};

/// The things in a cell of the land, in the cell's order
using CellContents = std::function<std::span<const HealCandidate>(glm::ivec2 cell)>;

/// How many cells across the heal looks: twice its radius over the cell's size, rounded up
[[nodiscard]] size_t HealCellsAcross(float radius);

/// The people a heal of a radius and most targets cast at a point heals, in the order it finds them
[[nodiscard]] std::vector<entt::entity> FindHealTargets(glm::vec3 point, float radius, size_t maximum,
                                                        const CellContents& cells);

} // namespace openblack::magic
