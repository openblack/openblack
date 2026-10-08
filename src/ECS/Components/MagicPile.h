/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "Enums.h"

namespace openblack::ecs::components
{

/// A pile of food or wood a miracle put down, and the player whose it is
struct MagicPile
{
	ResourceType resource {ResourceType::Food};
	PlayerNames player {PlayerNames::NEUTRAL};
};

} // namespace openblack::ecs::components
