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

#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>

namespace openblack::ecs::components
{

/// What a flock miracle keeps: its flock, how many animals it has made and should have made by now along the hand's
/// sweep, where the sweep last was, and the trail its caster's hand leaves while it makes them
struct FlockSpell
{
	entt::entity flock {entt::null};
	int created {0};
	float emitted {0.0f};
	/// The hand's point across the land and its height above the land when the last animals were made
	glm::vec2 lastSpawn {0.0f};
	float lastSpawnHeight {0.0f};
	/// The hand's trail, this computer's caster's alone; 0 for none
	uint32_t castEffect {0};
	bool castEffectStarted {false};
	/// Doves or bats, as the caster's alignment was at the cast
	bool evil {false};
	/// The animals there when the miracle closed down have been told to fade out
	bool closedDown {false};
};

} // namespace openblack::ecs::components
