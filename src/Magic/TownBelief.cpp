/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TownBelief.h"

#include <algorithm>

using namespace openblack;
using namespace openblack::magic::town_belief;

void magic::town_belief::Add(Belief& town, PlayerNames player, float amount)
{
	const auto p = static_cast<size_t>(player);
	if (p >= k_PlayerCount)
	{
		return;
	}
	town.pending.at(p) += amount;
	town.recent.at(p) += amount;
}

std::vector<Gained> magic::town_belief::Turn(Belief& town, float recentDecay)
{
	std::vector<Gained> gained;
	for (size_t p = 0; p < k_PlayerCount; ++p)
	{
		town.recent.at(p) *= recentDecay;
		const float amount = town.scale * town.pending.at(p);
		if (amount == 0.0f)
		{
			continue;
		}
		gained.push_back({.player = static_cast<PlayerNames>(p), .amount = amount});
		// Capped from above only: belief can fall below nothing
		town.belief.at(p) = std::min(town.belief.at(p) + amount, town.cap.at(p));
		town.pending.at(p) = 0.0f;
	}
	return gained;
}
