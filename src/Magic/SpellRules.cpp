/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "SpellRules.h"

#include <algorithm>

#include "InfoConstants.h"
#include "MagicTables.h"

using namespace openblack;
using namespace openblack::magic;

namespace
{
/// A fire seed throws a fireball of this size whatever the gesture
constexpr float k_FireSeedMagnitude = 1.0f;
/// The water miracle's rain radius and ring growth, then its power-up's, and anything else's
constexpr float k_WaterRainRadius = 6.0f;
constexpr float k_WaterPowerUpRainRadius = 12.0f;
constexpr float k_WaterRippleGrowth = 2.0f;
constexpr float k_WaterPowerUpRippleGrowth = 4.0f;
} // namespace

SpellClass magic::ClassOf(MagicType type)
{
	using S = MagicInfoSection;
	switch (SlotOf(type).section)
	{
	case S::General:
		return SpellClass::General;
	case S::Heal:
		return SpellClass::Heal;
	case S::Teleport:
		return SpellClass::Teleport;
	case S::Forest:
		return SpellClass::Forest;
	case S::Food:
	case S::Wood:
		return SpellClass::Resource;
	case S::StormAndTornado:
		return SpellClass::StormAndTornado;
	case S::Shield:
		return SpellClass::Shield;
	case S::Water:
		return SpellClass::Water;
	case S::FlockFlying:
		return SpellClass::FlockFlying;
	case S::FlockGround:
		return SpellClass::FlockGround;
	case S::CreatureSpell:
	case S::_COUNT:
		break;
	}
	return SpellClass::Creature;
}

SpellCastData magic::SeedCastData(const InfoConstants& info, MagicType type, SpellSeedType seed, float multiplier,
                                  float gestureSize)
{
	const float timer = GetTimerWhenPlayerCasting(info, type);
	return {
	    .magnitude = seed == SpellSeedType::Fire ? k_FireSeedMagnitude : gestureSize,
	    .chants = GetMagicEffectInfo(info, type).initialChants * multiplier,
	    .duration = timer < 0.0f ? k_NoTimeLimit : timer * multiplier,
	    .maxObjectsToCreate = -1,
	};
}

bool magic::AgeOneTurn(float& age, float duration, float seconds)
{
	age += seconds;
	return duration >= 0.0f && age > duration;
}

void EffectValues::Scale(float factor)
{
	for (auto& number : numbers)
	{
		number *= factor;
	}
}

bool EffectValues::IsDestructive() const
{
	return (*this)[EffectKind::Burn] > 0.0f || (*this)[EffectKind::Crush] > 0.0f || (*this)[EffectKind::Hit] > 0.0f ||
	       (*this)[EffectKind::FlyAway] > 0.0f;
}

EffectValues EffectValues::From(const GEffectInfo& info)
{
	return {
	    .numbers = {info.effectBurn, info.effectCrush, info.effectHit, info.effectHeal, info.effectFlyAway,
	                info.effectAlignmentModification, info.effectBeliefModification},
	    .radius = info.radius,
	};
}

EffectDefence EffectDefence::From(const GObjectInfo& info)
{
	return {
	    .multipliers = {info.defenceMultiplierBurn, info.defenceMultiplierCrush, info.defenceMultiplierHit,
	                    info.defenceMultiplierHeal, info.defenceMultiplierFlyAway, info.defenceMultiplierAlignmentModification,
	                    info.defenceMultiplierBeliefModification},
	    .combustionTemperature = info.combustionTemperature,
	};
}

EffectDefence magic::CreatureDefence(EffectDefence species, float size)
{
	const float divisor = (std::clamp(size, 0.001f, 2.0f) * 1.5f) + 1.0f;
	for (const auto kind : {EffectKind::Burn, EffectKind::Crush, EffectKind::Hit, EffectKind::FlyAway})
	{
		species.multipliers.at(static_cast<size_t>(kind)) /= divisor;
	}
	return species;
}

float magic::DamageFrom(const EffectValues& values, const EffectDefence& defence)
{
	float damage = 0.0f;
	for (const auto kind : {EffectKind::Crush, EffectKind::Hit})
	{
		damage += std::max(values[kind] * defence.multipliers.at(static_cast<size_t>(kind)), 0.0f);
	}
	return damage;
}

float magic::HealFrom(const EffectValues& values, const EffectDefence& defence)
{
	return std::max(values[EffectKind::Heal] * defence.multipliers.at(static_cast<size_t>(EffectKind::Heal)), 0.0f);
}

float magic::HeatDamage(float temperature, const EffectDefence& defence)
{
	const float combustion = defence.combustionTemperature;
	if (combustion <= 0.0f || temperature < combustion)
	{
		return 0.0f;
	}
	return (temperature - combustion) / combustion * defence.multipliers.at(static_cast<size_t>(EffectKind::Burn)) *
	       k_HeatDamageScale;
}

float magic::LifeAfter(float life, const EffectValues& values, const EffectDefence& defence)
{
	life = std::min(life + HealFrom(values, defence), 1.0f);
	return std::max(life - DamageFrom(values, defence), 0.0f);
}

ResourceEventCost magic::ResourceEvent(const GMagicResourceInfo& info, bool firstDone)
{
	const uint32_t units = firstDone ? info.resourceAmountPerEvent : info.resourceAmountFirstEvent;
	return {units, static_cast<float>(static_cast<int64_t>(info.costPerUnit) * static_cast<int64_t>(units))};
}

bool magic::HasEnoughChantsForResourceRecast(const GMagicResourceInfo& info, float chants)
{
	return static_cast<float>(info.costPerUnit * info.resourceAmountFirstEvent) <= chants;
}

float magic::ClampShieldRadius(const GMagicShieldInfo& info, float radius)
{
	return std::max(std::min(radius, info.maxRadius), info.minRadius);
}

float magic::ShieldCostToMaintain(float plainCost, float radius, float radiusForNormalCost)
{
	if (radiusForNormalCost <= 0.0f)
	{
		return plainCost;
	}
	const float share = radius / radiusForNormalCost;
	return plainCost * share * share;
}

float magic::RainRadius(MagicType type)
{
	switch (type)
	{
	case MagicType::Water:
		return k_WaterRainRadius;
	case MagicType::WaterPowerUpOne:
		return k_WaterPowerUpRainRadius;
	default:
		return 1.0f;
	}
}

float magic::RippleGrowth(MagicType type)
{
	switch (type)
	{
	case MagicType::Water:
		return k_WaterRippleGrowth;
	case MagicType::WaterPowerUpOne:
		return k_WaterPowerUpRippleGrowth;
	default:
		return 1.0f;
	}
}
