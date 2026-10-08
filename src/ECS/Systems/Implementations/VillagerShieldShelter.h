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

namespace openblack::ecs::components
{
struct LivingAction;
}

/// A villager amazed by a shield: under it, it turns to face out and looks about, now and then taking up a new
/// animation, for as long as its town wants protection and the shield stands; then it waits a little and decides what
/// to do. The state functions are the living action system's.
namespace openblack::ecs::villager_shield
{

/// How much a town wants protection beyond what has its villagers act on it, 0 when not at all
[[nodiscard]] float ProtectionSignificance(entt::entity town);

uint32_t AmazedByMagicShield(components::LivingAction& action);
/// Waiting out a count of turns before going on to its next state
uint32_t WaitForCounter(components::LivingAction& action);

} // namespace openblack::ecs::villager_shield
