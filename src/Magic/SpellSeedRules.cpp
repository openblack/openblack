/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "SpellSeedRules.h"

#include <algorithm>

#include "InfoConstants.h"

using namespace openblack;
using namespace openblack::magic;

CastStyle magic::CastStyleOf(SpellCastType castType)
{
	switch (castType)
	{
	case SpellCastType::SpellCastInHand:
		return CastStyle::Held;
	case SpellCastType::SpellCastHandGesture:
		return CastStyle::OnRelease;
	case SpellCastType::SpellCastHandPosition:
		break;
	}
	return CastStyle::OnPress;
}

CastStyle magic::CastStyleOf(const GSpellSeedInfo& seed)
{
	return CastStyleOf(seed.castType);
}

float magic::ChantNeeded(float costToCreate, float store)
{
	return costToCreate - store;
}

float magic::SeedPower(float store, float costToCreate)
{
	return costToCreate > 0.0f ? std::min(store / costToCreate, 1.0f) : 1.0f;
}

bool magic::SeedReadyAfter(uint32_t turnsInHand, float turnSeconds, float delaySeconds)
{
	return static_cast<float>(turnsInHand) * turnSeconds > delaySeconds;
}
