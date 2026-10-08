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
#include <vector>

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
	/// A forest of the land is made about a point: with the land's script's number for it, or else the next number
	/// (a number given moves the count past it); a big forest's own, or a town's forest of the lone trees about it. The
	/// newest forest is met first. Its number.
	virtual uint32_t MakeLandForest(std::optional<uint32_t> id, glm::vec3 position, entt::entity bigForest, bool scenic) = 0;
	/// The first forest met with a number, none when there is none
	[[nodiscard]] virtual std::optional<entt::entity> LandForestOf(uint32_t id) const = 0;
	/// The land's forests, the newest first
	[[nodiscard]] virtual std::vector<entt::entity> LandForests() const = 0;
	/// A forest's grown or still growing trees, the nearest its place first (the older first when as near)
	[[nodiscard]] virtual std::vector<entt::entity> TreesOf(entt::entity forest, bool growing) const = 0;
	/// The wood in a forest: its big forest's and every tree's
	[[nodiscard]] virtual float WoodOf(entt::entity forest) const = 0;
	/// The point of a forest nearest a point: on its big forest's edge towards it, or else the forest's place
	[[nodiscard]] virtual glm::vec3 NearestPointOf(entt::entity forest, glm::vec3 to) const = 0;
	/// Each town lists the forests near enough its storage pit with wood in them
	virtual void AssignForestsToTowns() = 0;
	/// Where a tiger's or a wolf's flock makes its home, choosing among the forests from a point; none to keep it
	[[nodiscard]] virtual std::optional<glm::vec3> ForestLair(AnimalInfo kind, glm::vec3 from) const = 0;

	/// Once the land is laid out: each town gathers the lone trees about it, out to a little beyond the reach of its
	/// forests, into a forest of its own (taking a tree from another town's such forest when it stands nearer this town)
	virtual void MakeScenicForests() = 0;

	/// Once a game turn, after the miracles: the forests grow or wither
	virtual void ProcessTurn() = 0;
	/// A new land: no forests
	virtual void Reset() = 0;
};

} // namespace openblack::ecs::systems
