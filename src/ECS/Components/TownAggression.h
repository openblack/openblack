/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/TownAggression.h"

namespace openblack::ecs::components
{

/// On a town: what it keeps of the attacks on it (see TownAggression.h)
struct TownAggression
{
	town_aggression::Record record;
};

} // namespace openblack::ecs::components
