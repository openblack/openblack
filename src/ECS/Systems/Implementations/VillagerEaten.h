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

namespace openblack::ecs::components
{
struct LivingAction;
}

/// A villager a hunter brought down: it lies where it fell while it is eaten, and then dead. The animals' system counts
/// the turns and moves it on; these are the living action system's state functions for the while.
namespace openblack::ecs::villager_eaten
{

/// Brought down, being eaten, or dead: it lies still
uint32_t LiesStill(components::LivingAction& action);

} // namespace openblack::ecs::villager_eaten
