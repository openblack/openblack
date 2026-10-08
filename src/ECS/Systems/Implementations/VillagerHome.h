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

#include <entt/entity/fwd.hpp>
#include <glm/vec2.hpp>

#include "Enums.h"

namespace openblack::ecs::components
{
struct LivingAction;
}

/// A villager's day around its home: deciding what to do, going home, staying in and sleeping. Villagers take up what
/// their town wants most that they can serve; at night that is sleep, which sends them home to bed. The state functions
/// are the living action system's.
namespace openblack::ecs::villager_home
{

/// Walks the villager to a point, then on into a state
void SetupMoveTo(components::LivingAction& action, glm::vec2 goal, VillagerStates final);
/// The villager's walk to a goal starts afresh, ending in a final state, whatever state it is in meanwhile
void SetupMobileMoveTo(components::LivingAction& action, glm::vec2 goal, VillagerStates final);

/// The villager goes into its abode, and isn't drawn while there
void ArriveHome(entt::entity villager);
/// It comes out again
void LeaveHome(entt::entity villager);

/// Sends the villager to bed if it is home, or home if it has one. 1 when it went.
uint32_t CheckSatisfySleep(components::LivingAction& action);

uint32_t DecideWhatToDo(components::LivingAction& action);
uint32_t MoveToPos(components::LivingAction& action);
uint32_t GoHome(components::LivingAction& action);
uint32_t ArrivesHome(components::LivingAction& action);
uint32_t AtHome(components::LivingAction& action);
uint32_t GotoBedAtHome(components::LivingAction& action);
uint32_t SleepingAtHome(components::LivingAction& action);
/// Leaving a home state: the villager comes out unless the next state keeps it inside
bool ExitAtHome(components::LivingAction& action, VillagerStates next);

} // namespace openblack::ecs::villager_home
