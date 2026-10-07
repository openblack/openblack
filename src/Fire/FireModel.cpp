/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "FireModel.h"

#include <cmath>

#include <algorithm>

using namespace openblack;
using namespace openblack::fire;

namespace
{
/// The cooling counts the air as this much colder than it is, so that something just above it still cools
constexpr float k_CoolingOffset = 10.0f;
/// The share of the surface and capacity that cools each turn
constexpr float k_CoolingRate = 0.1f;
/// A burning thing heats itself by this share of its temperature over twice its combustion temperature each turn
constexpr float k_SelfHeating = 0.1f;
/// The smallest gap the flames' height is measured over
constexpr float k_SmallestGap = 0.0001f;
} // namespace

float fire::CombustionTemperature(const Material& material)
{
	return std::max(material.combustion, k_LowestCombustion);
}

float fire::MaxTemperature(const Material& material)
{
	return 2.0f * CombustionTemperature(material);
}

float fire::Capacity(const Material& material)
{
	return std::max(material.capacity, 1.0f);
}

float fire::CoolingArea(const Material& material)
{
	return material.height * material.radius * 4.0f;
}

bool fire::IsOnFire(float temperature, const Material& material)
{
	return temperature >= CombustionTemperature(material);
}

bool fire::IsAboveReactionTemperature(float temperature, const Material& material)
{
	return temperature >= k_ReactionTemperature || IsOnFire(temperature, material);
}

float fire::FireFraction(float temperature, const Material& material, float life)
{
	const float combustion = CombustionTemperature(material);
	const float start = k_FractionStart * combustion;
	float fraction = (temperature - start) / (MaxTemperature(material) - start);
	// Something nearly burnt away burns weakly, however hot it is
	if (!(2.0f * life > fraction))
	{
		fraction = 2.0f * life;
	}
	return std::clamp(fraction, 0.0f, 1.0f);
}

float fire::FireRadius(const Material& material, float fraction)
{
	return material.fireRadius * k_FireRadiusScale * std::clamp(fraction, 0.0f, 1.0f);
}

float fire::MaxFireRadius(const Material& material)
{
	return material.fireRadius * k_FireRadiusScale;
}

float fire::SafeFireRadius(float radius, float maxRadius)
{
	return std::min(radius, maxRadius) + 1.0f;
}

float fire::FlameHeight(float temperature, const Material& material)
{
	const float gap = std::max(MaxTemperature(material) - k_AmbientTemperature, k_SmallestGap);
	return material.height * k_FlameHeightScale * std::clamp((temperature - k_AmbientTemperature) / gap, 0.0f, 1.0f);
}

float fire::HeatContent(float temperature, const Material& material)
{
	return (temperature - k_AmbientTemperature) * Capacity(material);
}

float fire::AddHeat(float temperature, const Material& material, float heat, float limit)
{
	float change = heat / Capacity(material);
	if (std::abs(limit) < std::abs(change))
	{
		change = limit;
	}
	return temperature + change;
}

float fire::HeatFromDifference(float difference)
{
	return k_HeatPerDegree * difference;
}

float fire::ApplyBurn(float temperature, const Material& material, float burn)
{
	const float difference = k_AmbientTemperature + burn - temperature;
	return AddHeat(temperature, material, HeatFromDifference(difference), difference);
}

float fire::SetOnFireTemperature(const Material& material, float speed)
{
	return MaxTemperature(material) * speed + CombustionTemperature(material);
}

float fire::BurnDamage(float temperature, const Material& material)
{
	const float combustion = CombustionTemperature(material);
	return (temperature - combustion) / (MaxTemperature(material) - combustion) * material.defence * k_DamageScale;
}

Transfer fire::HeatTransfer(float sourceTemperature, const Material& source, float targetTemperature, const Material& target)
{
	Transfer result {.sourceTemperature = sourceTemperature, .targetTemperature = targetTemperature};
	const float difference = sourceTemperature - targetTemperature;
	if (!(difference > 0.0f))
	{
		return result;
	}
	const float heat = std::min(HeatFromDifference(difference), HeatContent(sourceTemperature, source) * 0.5f);
	result.targetTemperature = AddHeat(targetTemperature, target, heat, difference);
	// Something hot that doesn't burn, such as a fireball below its own combustion, cools as it heats
	if (!IsOnFire(sourceTemperature, source))
	{
		result.sourceTemperature = AddHeat(sourceTemperature, source, -heat, -difference);
	}
	return result;
}

TurnOutcome fire::Step(State& state, const Material& material, const Surroundings& surroundings)
{
	TurnOutcome outcome;
	if (state.temperature - k_AmbientTemperature < k_GoneAboveAmbient && state.charring == 0.0f)
	{
		outcome.gone = true;
		return outcome;
	}
	state.flags &= static_cast<uint8_t>(~k_TurnFlags);
	const float combustion = CombustionTemperature(material);
	if (state.temperature > k_VeryHotScale * combustion)
	{
		state.flags |= k_VeryHot;
	}
	if (state.previous < combustion && state.temperature >= combustion)
	{
		state.flags |= k_JustIgnited;
	}
	const bool notBurning = state.temperature < combustion || material.defence == 0.0f;

	float multiplier = 1.0f;
	bool heats = false;
	if (surroundings.inWater)
	{
		multiplier = k_WaterCooling;
		state.flags |= k_Cooling;
	}
	else if (surroundings.rain > 0.0f && material.rainCooling > 0.0f)
	{
		multiplier = material.rainCooling * surroundings.rain + 1.0f;
		state.flags |= k_Cooling;
		outcome.rainedOn = true;
	}
	else
	{
		if (state.temperature < state.previous)
		{
			state.flags |= k_Cooling;
		}
		heats = !notBurning;
	}

	if (heats)
	{
		// Burning, it slowly heats itself up to twice its combustion temperature
		state.temperature += k_SelfHeating * state.temperature / MaxTemperature(material);
		state.temperature = std::min(state.temperature, MaxTemperature(material));
	}
	else
	{
		state.temperature = std::max(state.temperature, k_AmbientTemperature);
		// Only when nothing heated it this turn
		if (!(state.temperature > state.previous))
		{
			state.temperature -= (state.temperature + k_CoolingOffset - k_AmbientTemperature) * CoolingArea(material) *
			                     k_CoolingRate * multiplier / Capacity(material);
			if (state.previous >= combustion && state.temperature < combustion)
			{
				state.flags |= k_JustExtinguished;
			}
		}
	}

	const float charLimit = std::max((k_CharLife - surroundings.life) * (1.0f / k_CharLife), 0.0f);
	if (state.temperature >= combustion)
	{
		if (surroundings.life < k_CharLife)
		{
			state.charring = std::min(std::min(state.charring + k_CharRate, 1.0f), charLimit);
		}
		if (surroundings.life > 0.0f)
		{
			outcome.damage = BurnDamage(state.temperature, material);
		}
	}
	else if (state.charring != 0.0f && surroundings.life != 0.0f)
	{
		state.charring = std::min(std::max(state.charring - k_UncharRate, 0.0f), charLimit);
	}
	return outcome;
}
