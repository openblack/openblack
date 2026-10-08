/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>
#include <string_view>

#include <entt/core/hashed_string.hpp>
#include <entt/entity/entity.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"
#include "Magic/MagicTables.h"

namespace openblack::ecs::components
{

/// A one-shot miracle lying about: a globe with its miracle spinning inside. Tapping it puts the miracle in the hand,
/// fully charged, and the globe pops. The globe's model is a dome, turned every frame to face the camera about its
/// middle so that it looks round from everywhere, with a glint running over it. Inside it the miracle's seed spins, or
/// its effect plays for the miracles shown only by one; an extreme miracle has a ring round it for each power-up.
struct OneOffSpellSeed
{
	static constexpr std::string_view k_MeshFile = "Spells/Meshes/O_bibble_up.l3d";
	static constexpr std::string_view k_RingMeshFile = "Spells/Meshes/Power_Up_Band.L3d";
	static constexpr auto k_MeshId = entt::hashed_string("spells/o_bibble_up");
	static constexpr auto k_RingMeshId = entt::hashed_string("spells/power_up_band");

	SpellSeedType seedType {SpellSeedType::None};
	/// The miracle it was made to give, none for one made from a seed alone
	MagicType magicType {MagicType::None};
	/// Where it floats; its transform turns about its middle from here to face the camera
	glm::vec3 position {0.0f};
	int powerUp {magic::k_BasePowerUpLevel};
	/// Scales the prayer power and the time of what it casts
	float multiplier {1.0f};
	/// The middle of the globe, where its miracle spins
	glm::vec3 middle {0.0f};
	/// How far the seed inside has spun, its rings have spun, and the glint has run on (a cell of its sheet)
	float spin {0.0f};
	float ringSpin {0.0f};
	float glintFrame {0.0f};
	/// A creature spell's phial runs through its texture and pulses
	float phialFrame {0.0f};
	float phialPhase {0.0f};
	/// How the seed's model was placed when last drawn, scale and all, which its effect's glints sparkle on; none before
	/// it has been
	std::optional<glm::mat4> seedPlacement;
	/// Where its dispenser made it, none for one put down by itself
	entt::entity dispenser {entt::null};
};

} // namespace openblack::ecs::components
