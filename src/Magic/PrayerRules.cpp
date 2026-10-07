/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "PrayerRules.h"

#include <algorithm>

using namespace openblack;
using openblack::ecs::components::PrayerPower;

float magic::DrawPrayer(PrayerPower& store, float amount)
{
	if (!(amount > 0.0f))
	{
		return 0.0f;
	}
	if (store.infinite)
	{
		return amount;
	}
	const float given = std::min(amount, std::max(store.chants, 0.0f));
	store.chants -= given;
	return given;
}

void magic::ReturnPrayer(PrayerPower& store, float amount)
{
	if (amount > 0.0f && !store.infinite)
	{
		store.chants += amount;
	}
}

float magic::ChargeSeed(PrayerPower& store, float costToCreate)
{
	return DrawPrayer(store, costToCreate);
}

float magic::SeedRefund(bool hasIcon, float chantStore, float storedChants, bool hasCast)
{
	if (!hasIcon)
	{
		return 0.0f;
	}
	if (storedChants >= 0.0f)
	{
		return storedChants;
	}
	return hasCast ? 0.0f : std::max(chantStore, 0.0f);
}

float magic::PlayerMaintainSpell(PlayerNames player, PrayerPower* store, float amount)
{
	if (player == PlayerNames::NEUTRAL)
	{
		return std::max(amount, 0.0f);
	}
	return store != nullptr ? DrawPrayer(*store, amount) : 0.0f;
}
