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

/// A pile of food or wood a miracle put down, which more of the same miracle tops up
struct MagicPile
{
	ResourceType resource {ResourceType::Food};
	/// Its food makes the people who eat it work faster
	bool sparkles {false};
};

} // namespace openblack::ecs::components
