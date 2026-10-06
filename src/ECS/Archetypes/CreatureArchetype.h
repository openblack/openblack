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
class CreatureArchetype
{
public:
	/// What a creature has become: alignment from -1 (evil) to 1 (good), fatness and strength from 0 to 1. Its body is
	/// drawn from these, its species' own strength counting a little towards how strong it looks.
	struct Body
	{
		float alignment {0.0f};
		float fatness {0.5f};
		float strength {0.5f};
	};

	/// How a new creature of the species starts: neutral, as big, fat and strong as the species starts
	[[nodiscard]] static float StartScale(CreatureType species);
	[[nodiscard]] static Body StartBody(CreatureType species);
	/// The scale a creature of the species is drawn at for its size, by its base mesh's rest pose
	[[nodiscard]] static float DrawnScale(CreatureType species, float size);
	/// The species' own strength, 0 to 1, which counts a little towards how strong its creatures look
	[[nodiscard]] static float SpeciesStrength(CreatureType species);

	static entt::entity Create(const glm::vec3& position, PlayerNames playerName, CreatureType creatureType,
	                           entt::id_type creatureMindId, float yAngleRadians, float scale, const Body& body = {});
	CreatureArchetype() = delete;
};
} // namespace openblack::ecs::archetypes
