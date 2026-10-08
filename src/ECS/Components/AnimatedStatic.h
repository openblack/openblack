/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <string>

#include "Enums.h"

namespace openblack::ecs::components
{

struct AnimatedStatic
{
	AnimatedStaticInfo type;
	/// Its state words, which pick the model a gate collides with: whether it stands open (1), and whether a gate stone
	/// plinth has its stones set and all of them. Nothing opens gates or fills plinths in openblack yet, so they stay 0.
	int32_t openState {0};
	int32_t plinthState {0};
	int32_t plinthFull {0};
};

} // namespace openblack::ecs::components
