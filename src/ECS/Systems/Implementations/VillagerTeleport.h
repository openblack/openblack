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

#include "Enums.h"

namespace openblack::ecs::components
{
struct LivingAction;
}

/// A villager turning aside into a teleport stone: it walks (or runs, when it was already going faster than its walk)
/// to the stone, jumps to the stone that leaves it closest to where it was going, then takes up again what it was doing
/// as its state table says. The state functions are the living action system's.
namespace openblack::ecs::villager_teleport
{

/// Heading for the stone, walking or running
uint32_t GoTowardsTeleportReaction(components::LivingAction& action);
/// At the stone: the jump, then on its way again
uint32_t TeleportReaction(components::LivingAction& action);

/// The state a villager keeps to come back to as it starts reacting to a stone, from the state it was to take
[[nodiscard]] VillagerStates PreviousToKeep(VillagerStates final);
/// The villager stops reacting to its stone: it takes the state its table says to come back to from the state it kept
void StopReacting(components::LivingAction& action, entt::entity villager);

} // namespace openblack::ecs::villager_teleport
