/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "SnowSystem.h"

#include <algorithm>

using namespace openblack::ecs::systems;
using openblack::ecs::components::Storm;

namespace
{
/// A game turn, in seconds
constexpr float k_TurnSeconds = 0.1f;
} // namespace

SnowSystem::SnowSystem()
    : _depths(snow_cover::k_Cells, 0.0f)
{
}

void SnowSystem::Reset()
{
	std::ranges::fill(_depths, 0.0f);
	_melting = {};
	++_revision;
}

void SnowSystem::ProcessTurn(std::span<const Storm> storms)
{
	for (const auto& storm : storms)
	{
		const auto amount = snow_cover::StormSnowPerTurn(storm.effect.snow, storm.currentStrength);
		if (amount != 0.0f)
		{
			snow_cover::AddStorm(_depths, {storm.currentPosition.x, storm.currentPosition.z}, storm.currentInnerRadius,
			                     storm.outerRadius, amount);
		}
	}
	snow_cover::Melt(_depths, _melting, k_TurnSeconds);
	++_revision;
}
