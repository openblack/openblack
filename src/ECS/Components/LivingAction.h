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
	static constexpr std::array<std::string_view, static_cast<size_t>(Index::_Count)> k_IndexStrings = {
	    "Top",
	    "Final",
	    "Previous",
	};

	LivingAction(VillagerStates topState, uint16_t turnsUntilStateChange)
	    : states {static_cast<uint8_t>(topState), static_cast<uint8_t>(VillagerStates::InvalidState),
	              static_cast<uint8_t>(VillagerStates::InvalidState)}
	    , turnsUntilStateChange(turnsUntilStateChange)
	    , turnsSinceStateChange(0)
	{
	}

	// Likely for animals
	LivingAction(LivingStates topState, uint16_t turnsUntilStateChange)
	    : states {static_cast<uint8_t>(topState), static_cast<uint8_t>(LivingStates::LivingInvalid),
	              static_cast<uint8_t>(LivingStates::LivingInvalid)}
	    , turnsUntilStateChange(turnsUntilStateChange)
	    , turnsSinceStateChange(0)
	{
	}

	// LivingState or VillagerState
	std::array<uint8_t, static_cast<size_t>(Index::_Count)> states;
	uint16_t turnsUntilStateChange;
	uint16_t turnsSinceStateChange;
};
} // namespace openblack::ecs::components
