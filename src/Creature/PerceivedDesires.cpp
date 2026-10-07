/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "PerceivedDesires.h"

#include <algorithm>

using namespace openblack;
using namespace openblack::creature_perceived_desires;

namespace
{
constexpr uint16_t k_GameAngleMask = 0x7FF;
constexpr uint16_t k_HalfTurn = 0x400;
constexpr uint16_t k_Turn = 0x800;
} // namespace

void creature_perceived_desires::Increase(PerceivedDesires& desires, size_t desire, float amount)
{
	if (desire < desires.player.size())
	{
		desires.player.at(desire) = std::clamp(desires.player.at(desire) + amount, 0.0f, 1.0f);
	}
}

void creature_perceived_desires::IncreaseTown(PerceivedDesires& desires, size_t desire, float amount)
{
	if (desire < desires.town.size())
	{
		desires.town.at(desire) = std::clamp(desires.town.at(desire) + amount, 0.0f, 1.0f);
	}
}

void creature_perceived_desires::Fade(PerceivedDesires& desires)
{
	std::ranges::for_each(desires.player, [](float& value) { value *= k_TurnFade; });
	std::ranges::for_each(desires.town, [](float& value) { value *= k_TurnFade; });
}

std::optional<size_t> creature_perceived_desires::TakeDominant(PerceivedDesires& desires,
                                                               const std::function<bool(size_t)>& activated)
{
	std::optional<size_t> dominant;
	for (size_t desire = 0; desire < desires.player.size(); ++desire)
	{
		if (activated(desire) && desires.player.at(desire) > 0.0f)
		{
			desires.player.at(desire) = 0.0f;
			dominant = desire;
		}
	}
	return dominant;
}

bool creature_perceived_desires::CanSeePos(uint16_t lookAngle, uint16_t angleToPoint, bool sameCell)
{
	const auto a = static_cast<int32_t>(lookAngle & k_GameAngleMask);
	const auto b = static_cast<int32_t>(angleToPoint & k_GameAngleMask);
	auto difference = a > b ? a - b : b - a;
	if (difference > k_HalfTurn)
	{
		difference = k_Turn - difference;
	}
	return difference <= k_SeeHalfAngle || sameCell;
}
