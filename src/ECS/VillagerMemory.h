/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

namespace openblack::ecs::components
{
struct LivingAction;
}

/// What a villager remembers of what it was doing, to go back to it once something has taken it away from it
namespace openblack::ecs::villager_memory
{

/// The villager remembers what it was doing: the state it is in or walking to, unless that is one it shouldn't come back
/// to (a reaction, or one that keeps what was remembered before), when it keeps what it remembered
void StorePreviousState(components::LivingAction& action);

} // namespace openblack::ecs::villager_memory
