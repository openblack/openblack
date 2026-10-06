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

/// A tree or a field's crop, which sways with the wind. Trees also bend away from the hand (see VegetationSystem).
struct Swayable
{
	/// Which of the 16 sways it follows: a tree's is picked by its facing, a field's at random
	uint8_t swaySlot;
};

} // namespace openblack::ecs::components
