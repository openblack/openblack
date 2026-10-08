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

#include <optional>

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack::ecs::components
{
struct Spell;
}

namespace openblack::ecs::systems
{

/// The forests the forest miracle plants. A forest miracle plants all its trees at once on a
/// spiral round where it was cast, of the kinds the ground there grows; they grow a little each turn while the miracle
/// lasts, and wither away once it has gone, the forest going with its last tree. They also grow by themselves as any tree
/// short of its size does, which the vegetation system sees to.
class ForestSystemInterface
{
public:
	virtual ~ForestSystemInterface() = default;

	/// The forest miracle plants its trees, their wood worth more by its caster's tribal power; how many it planted
	virtual uint32_t Plant(entt::entity spell, components::Spell& miracle, float tribalPower) = 0;
	/// Whether a forest may be planted at a point: no building there whose fire would be in it, and room for a tree
	[[nodiscard]] virtual bool CanGrowAt(glm::vec3 point) const = 0;
	/// Whether a forest miracle's forest still has trees
	[[nodiscard]] virtual bool HasTrees(entt::entity spell) const = 0;
	/// A young tree of the same kind is planted near a tree of a forest, as the water miracle plants one by a full grown
	/// tree: no sooner than 41 turns after the last tree any forest gained this way (none in a land's first 41 turns), at
	/// the first free spot of up to 160 tried round it. One planted by a forest miracle's tree joins that miracle's
	/// forest and goes with it. The tree planted, none when it is too soon, the tree is in no forest, or no spot is free.
	virtual std::optional<entt::entity> AddTreeNear(entt::entity tree) = 0;
	/// The number a new forest of the land takes, as a tree put down away from any forest starts one: the next after the
	/// land's own forests, never one the forest miracles' forests are numbered with
	[[nodiscard]] virtual uint32_t NewLandForestId() const = 0;

	/// Once a game turn, after the miracles: the forests grow or wither
	virtual void ProcessTurn() = 0;
	/// A new land: no forests
	virtual void Reset() = 0;
};

} // namespace openblack::ecs::systems
