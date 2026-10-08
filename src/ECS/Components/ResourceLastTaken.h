/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>
#include <optional>

#include "Enums.h"

namespace openblack::ecs::components
{

/// On a town: the turn each player last took food and wood from its stores, which makes what they give it back count
/// for less belief for a while
struct ResourceLastTaken
{
	std::array<std::array<std::optional<uint32_t>, 2>, static_cast<size_t>(PlayerNames::_COUNT)> turn {};
};

} // namespace openblack::ecs::components
