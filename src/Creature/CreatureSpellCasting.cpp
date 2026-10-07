/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureSpellCasting.h"

#include <algorithm>

using namespace openblack::creature_spell_casting;

float openblack::creature_spell_casting::ChantsToEnergy(float chants, const Body& body, const Rates& rates)
{
	return chants / ((body.size * rates.sizeFactor + body.strength + 1.0f) * rates.chantsPerEnergy);
}

float openblack::creature_spell_casting::MostChants(const Body& body, const Rates& rates)
{
	return (body.size + body.strength + 1.0f) * (body.energy - rates.energyFloor) * rates.chantsPerEnergy;
}

float openblack::creature_spell_casting::MaintainSpell(Body& body, const Rates& rates, float amount, float energyShare)
{
	if (!(amount > 0.0f))
	{
		return amount;
	}
	const float most = MostChants(body, rates);
	if (!(most > 0.0f))
	{
		return 0.0f;
	}
	const float paid = amount < most ? amount : most;
	const float tired = std::clamp(ChantsToEnergy(paid, body, rates), 0.0f, k_MostTiredByPaying);
	body.energy = std::clamp(body.energy - tired * energyShare, 0.0f, std::max(body.size, 1.0f));
	body.exhaustion += tired;
	return paid;
}

bool openblack::creature_spell_casting::CanCast(const Body& body, const Rates& rates, float costToCreate)
{
	return k_TooExhaustedToCast - ChantsToEnergy(costToCreate, body, rates) > body.exhaustion;
}

float openblack::creature_spell_casting::StaminaCost(float costToCreate)
{
	return std::clamp(costToCreate / k_ChantsPerStamina, 0.0f, 1.0f);
}

bool openblack::creature_spell_casting::MayTry(float seen, float needed)
{
	const float share = seen / needed;
	return share > 1.0f || !(share < k_ShareToTry);
}

bool openblack::creature_spell_casting::TrySucceeds(float seen, float needed)
{
	const float share = seen / std::max(needed - 1.0f, 1.0f);
	return share > 1.0f || !(share < k_ShareToSucceed);
}

float openblack::creature_spell_casting::CastMagnitude(float targetSize, std::optional<float> targetCreatureScale,
                                                       bool fireSeed, float casterHeight)
{
	if (fireSeed)
	{
		return std::min(casterHeight / k_FireMagnitudeHeight, 1.0f);
	}
	if (targetCreatureScale.has_value())
	{
		return *targetCreatureScale * k_HeightOfSizeOne * k_MagnitudeOverSize;
	}
	return targetSize * k_MagnitudeOverSize;
}
