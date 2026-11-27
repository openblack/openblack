/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>

#include "Enums.h"

namespace openblack::ecs::components
{

struct CreatureState
{
	std::array<float, static_cast<size_t>(CreatureDesires::Invalid)> desireIncreaseTimes {};
};

} // namespace openblack::ecs::components
