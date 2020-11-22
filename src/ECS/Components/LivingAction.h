/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <array>
#include <string_view>

#include "Enums.h"

namespace openblack::ecs::components
{
struct LivingAction
{
	enum class Index : uint8_t
	{
		Top,
		Final,
		Previous,

		_Count,
	};
	static constexpr std::array<std::string_view, static_cast<size_t>(Index::_Count)> IndexStrings = {
	    "Top",
	    "Final",
	    "Previous",
	};

	explicit LivingAction(VillagerStates topState)
	    : states {static_cast<uint8_t>(topState), static_cast<uint8_t>(VillagerStates::InvalidState),
	              static_cast<uint8_t>(VillagerStates::InvalidState)}
	    , turnsSinceStateChange(0)
	{
	}

	// Likely for animals
	explicit LivingAction(LivingStates topState)
	    : states {static_cast<uint8_t>(topState), static_cast<uint8_t>(LivingStates::LivingInvalid),
	              static_cast<uint8_t>(LivingStates::LivingInvalid)}
	    , turnsSinceStateChange(0)
	{
	}

	// LivingState or VillagerState
	std::array<uint8_t, static_cast<size_t>(Index::_Count)> states;
	uint16_t turnsSinceStateChange;
};
} // namespace openblack::ecs::components
