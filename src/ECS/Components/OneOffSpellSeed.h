/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <string_view>

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"
#include "Magic/MagicTables.h"

namespace openblack::ecs::components
{

/// A one-shot miracle lying about: a bubble with its miracle's seed spinning inside. Tapping it puts the miracle in the
/// hand, fully charged, and the bubble pops. The bubble's model is a dome, turned every frame to face the camera about
/// its middle so that it looks round from everywhere.
struct OneOffSpellSeed
{
	static constexpr std::string_view k_MeshFile = "Spells/Meshes/O_bibble_up.l3d";

	SpellSeedType seedType {SpellSeedType::None};
	/// The miracle it was made to give, none for one made from a seed alone
	MagicType magicType {MagicType::None};
	/// Where it floats; its transform turns about its middle from here to face the camera
	glm::vec3 position {0.0f};
	int powerUp {magic::k_BasePowerUpLevel};
	/// Scales the prayer power and the time of what it casts
	float multiplier {1.0f};
	/// The seed drawn inside, and how far it has spun
	entt::entity seedGraphic {entt::null};
	float spin {0.0f};
	/// Where its dispenser made it, none for one put down by itself
	entt::entity dispenser {entt::null};
};

} // namespace openblack::ecs::components
