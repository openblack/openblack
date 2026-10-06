/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <entt/fwd.hpp>
#include <glm/fwd.hpp>

#include "Enums.h"

namespace openblack::ecs::archetypes
{

/// The miracles that lie about and are held: a seed in the hand, a dispenser's one-shot bubble, and the dispenser
class SpellSeedArchetype
{
public:
	/// A seed of a player's at a point, at its tables' scale; its model is shown by the hand when the seed shows one
	static entt::entity Create(const glm::vec3& position, SpellSeedType seedType, PlayerNames player, int powerUp,
	                           float multiplier);
	SpellSeedArchetype() = delete;
};

class OneOffSpellSeedArchetype
{
public:
	/// A bubble of a model with a seed's model spinning inside, at a point; none for a seed that doesn't exist
	static entt::entity Create(const glm::vec3& position, SpellSeedType seedType, int powerUp, float multiplier,
	                           entt::id_type bubbleMesh);
	OneOffSpellSeedArchetype() = delete;
};

class SpellDispenserArchetype
{
public:
	/// A dispenser building of a tribe's kind for a magic type, turned and scaled, active, with its tables' period
	static entt::entity Create(const glm::vec3& position, MagicType magicType, AbodeInfo building, float yAngleRadians,
	                           float scale);
	SpellDispenserArchetype() = delete;
};

} // namespace openblack::ecs::archetypes
