/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack::ecs::components
{
struct CreatureMindState;
} // namespace openblack::ecs::components

/// What a player's creature sees of what its player does
namespace openblack::ecs::creature_sight
{

/// The mind of the player's creature when it can see the point, from where it looks: none when the player has no
/// creature or it can't see there
[[nodiscard]] components::CreatureMindState* MindSeeing(PlayerNames player, glm::vec3 point);

/// The player's creature, if it can see where the player did something, feels with the player: it takes it the player
/// wants more of one of its desires, by an amount, kept between none and all
void EmpathiseWithPlayer(PlayerNames player, CreatureDesires desire, float amount, glm::vec3 point);

} // namespace openblack::ecs::creature_sight
