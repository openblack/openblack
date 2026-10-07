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

#include "Enums.h"

namespace openblack::ecs::components
{

struct Pot
{
	uint32_t amount;
	uint32_t maxAmount;
	/// What kind of pot or pile it is, which says what it holds
	PotInfo type {PotInfo::FoodPot};
};

} // namespace openblack::ecs::components
