/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "VillagerSpeed.h"

#include <algorithm>

using namespace openblack::ecs;
using namespace openblack::ecs::villager_speed;

namespace
{
constexpr float k_LeastIndianPower = 0.1f;
constexpr float k_BeliefBeyondNeutral = 0.1f;
constexpr size_t k_CrawlSpeed = 4;
constexpr size_t k_DefaultSpeed = 0;
constexpr size_t k_FleeingSpeed = 1;
constexpr float k_LeastLoadMod = 0.75f;
constexpr float k_MostTownNeedsMod = 0.5f;
constexpr size_t k_StoryLands = 6;
constexpr int32_t k_CreationSpread = 31;
constexpr int32_t k_CreationStep = 47;
constexpr int32_t k_CreationMiddle = 16;
constexpr float k_CreationShare = 0.01f;
constexpr float k_AgeSlowing = 0.2f;
constexpr float k_AgeShare = 0.1f;
constexpr float k_MostAgeSlowing = 0.4f;
constexpr float k_HungerSlowing = 0.1f;
constexpr float k_LifeSlowing = 0.1f;
constexpr float k_WomanSlowing = 0.2f;

float LoadMod(float fullLoadMod, float held, float most)
{
	const float mod = fullLoadMod + 1.0f - held / most;
	return mod < k_LeastLoadMod ? k_LeastLoadMod : std::min(mod, 1.0f);
}
} // namespace

int32_t villager_speed::StateSpeed(const Inputs& inputs, const FloatRandom& random)
{
	const float power = inputs.indianPower.has_value()
	                        ? (*inputs.indianPower > k_LeastIndianPower ? *inputs.indianPower : k_LeastIndianPower)
	                        : 1.0f;
	float scale = inputs.landSpeedBalance * power;
	if (inputs.belief.has_value())
	{
		const float beyond = inputs.belief->player - inputs.belief->neutral;
		const float believed = beyond > 0.0f ? beyond * k_BeliefBeyondNeutral : inputs.belief->player;
		float beliefScale = inputs.landBeliefSpeedScale;
		if (beliefScale == 1.0f)
		{
			if (inputs.multiplayer)
			{
				beliefScale = inputs.beliefSpeedScaleMultiPlayer;
			}
			else if (inputs.landNumber < k_StoryLands)
			{
				beliefScale = inputs.beliefSpeedScaleStory.at(inputs.landNumber);
			}
		}
		scale = scale * (beliefScale * believed + 1.0f);
	}
	const auto speedOf = [&inputs](size_t index) { return static_cast<float>(inputs.speeds.at(index)); };
	float speed = 0.0f;
	if (inputs.life <= inputs.lifeWhenCrawlsWounded)
	{
		return static_cast<int32_t>((random(0.2f) + 0.4f) * speedOf(k_CrawlSpeed) * scale);
	}
	if (inputs.life <= inputs.lifeWhenWalksWounded)
	{
		return static_cast<int32_t>((random(0.25f) + 0.5f) * speedOf(k_DefaultSpeed) * scale);
	}
	if (inputs.townInEmergency)
	{
		speed = (random(0.5f) + 0.75f) * speedOf(k_FleeingSpeed) * scale;
	}
	else
	{
		float townMod = 1.0f;
		if (inputs.townNeeds.has_value())
		{
			const float share = *inputs.townNeeds / inputs.divisorForTownNeedsSpeedMod;
			townMod = (share < 0.0f ? 0.0f : std::min(share, k_MostTownNeedsMod)) + inputs.baseForTownNeedsSpeedMod;
		}
		const float woodMod = LoadMod(inputs.speedModWhenFullLoadOfWood, inputs.woodHeld, inputs.maxWood);
		const float foodMod = LoadMod(inputs.speedModWhenFullLoadOfFood, inputs.foodHeld, inputs.maxFood);
		speed = speedOf(inputs.speedIndex) * foodMod * woodMod * townMod * scale;
	}
	if (inputs.foodSpeedUp)
	{
		speed *= inputs.foodPowerupIncrease;
	}
	return static_cast<int32_t>(speed);
}

float villager_speed::PersonalFactor(const Person& person)
{
	// The multiplication wraps as the game's whole numbers do
	const auto stepped =
	    static_cast<int32_t>(static_cast<uint32_t>(person.creationIndex) * static_cast<uint32_t>(k_CreationStep));
	float factor = static_cast<float>(stepped % k_CreationSpread - k_CreationMiddle) * k_CreationShare + 1.0f;
	if (person.age < person.grownUpAge)
	{
		factor -= std::min(static_cast<float>(person.grownUpAge - person.age) * k_AgeSlowing * k_AgeShare, k_MostAgeSlowing);
	}
	else if (person.age > person.oldAge)
	{
		factor -= std::min(static_cast<float>(person.age - person.oldAge) * k_AgeSlowing * k_AgeShare, k_MostAgeSlowing);
	}
	else
	{
		factor = factor - person.desireForFood * k_HungerSlowing;
		factor -= person.life * k_LifeSlowing;
		if (person.female)
		{
			factor -= k_WomanSlowing;
		}
	}
	return factor;
}

uint16_t villager_speed::FinalSpeed(int32_t speed, float factor)
{
	const auto scaled = static_cast<int32_t>(static_cast<float>(speed) * factor);
	return static_cast<uint16_t>(std::clamp(scaled, 0, 0xFFFF));
}
