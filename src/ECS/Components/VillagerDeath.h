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

/// A villager that has died: the turns it lies dead before it goes, and whether its flesh has gone yet, leaving the
/// skeleton
struct VillagerDeath
{
	uint32_t turnsLeft {0};
	bool skeleton {false};
};

} // namespace openblack::ecs::components
