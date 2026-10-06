/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "MagicTables.h"

#include <cctype>
#include <cstring>

#include <algorithm>
#include <array>
#include <numeric>

using namespace openblack;
using namespace openblack::magic;

namespace
{
/// How many records each section holds, in section order
constexpr std::array<uint8_t, static_cast<size_t>(MagicInfoSection::_COUNT)> k_SectionSizes {
    10, 2, 1, 1, 2, 3, 2, 1, 2, 1, 1, 16,
};
static_assert(std::accumulate(k_SectionSizes.begin(), k_SectionSizes.end(), 0u) == k_MagicTypeCount);

constexpr std::array<MagicInfoSlot, k_MagicTypeCount> k_Slots = [] {
	std::array<MagicInfoSlot, k_MagicTypeCount> slots {};
	size_t type = 0;
	for (size_t section = 0; section < k_SectionSizes.size(); ++section)
	{
		for (uint8_t index = 0; index < k_SectionSizes.at(section); ++index)
		{
			slots.at(type++) = {.section = static_cast<MagicInfoSection>(section), .index = index};
		}
	}
	return slots;
}();

/// Case-insensitive comparison with a fixed-size, zero-padded name
bool EqualsIgnoringCase(std::string_view text, const std::array<char, 0x30>& name)
{
	const auto length = strnlen(name.data(), name.size());
	return text.size() == length && std::ranges::equal(text, std::string_view(name.data(), length), [](char a, char b) {
		       return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
	       });
}

std::optional<SpellSeedType> FindSeed(const InfoConstants& info, auto&& predicate)
{
	const auto found = std::ranges::find_if(info.spellSeed, predicate);
	if (found == info.spellSeed.end())
	{
		return std::nullopt;
	}
	return static_cast<SpellSeedType>(std::distance(info.spellSeed.begin(), found));
}
} // namespace

MagicInfoSlot magic::SlotOf(MagicType type)
{
	return k_Slots.at(static_cast<size_t>(type));
}

const GMagicInfo& magic::detail::SectionRecord(const InfoConstants& info, MagicInfoSlot slot)
{
	switch (slot.section)
	{
	case MagicInfoSection::General:
		return info.magicGeneral.at(slot.index);
	case MagicInfoSection::Heal:
		return info.magicHeal.at(slot.index);
	case MagicInfoSection::Teleport:
		return info.magicTeleport.at(slot.index);
	case MagicInfoSection::Forest:
		return info.magicForest.at(slot.index);
	case MagicInfoSection::Food:
		return info.magicFood.at(slot.index);
	case MagicInfoSection::StormAndTornado:
		return info.magicStormAndTornado.at(slot.index);
	case MagicInfoSection::Shield:
		return info.magicShield.at(slot.index);
	case MagicInfoSection::Wood:
		return info.magicWood.at(slot.index);
	case MagicInfoSection::Water:
		return info.magicWater.at(slot.index);
	case MagicInfoSection::FlockFlying:
		return info.magicFlockFlying.at(slot.index);
	case MagicInfoSection::FlockGround:
		return info.magicFlockGround.at(slot.index);
	case MagicInfoSection::CreatureSpell:
	case MagicInfoSection::_COUNT:
		break;
	}
	return info.magicCreatureSpell.at(slot.index);
}

const GMagicInfo& magic::GetMagicInfo(const InfoConstants& info, MagicType type)
{
	return detail::SectionRecord(info, SlotOf(type));
}

const GMagicEffectInfo& magic::GetMagicEffectInfo(const InfoConstants& info, MagicType type)
{
	return info.magicEffect.at(static_cast<size_t>(type));
}

std::optional<MagicType> magic::FindMagicTypeByName(const InfoConstants& info, std::string_view name)
{
	// The name searched is the effect name of the magic type each record says it is
	for (size_t i = 0; i < k_MagicTypeCount; ++i)
	{
		const auto& record = GetMagicInfo(info, static_cast<MagicType>(i));
		if (EqualsIgnoringCase(name, GetMagicEffectInfo(info, record.magicType).debugString))
		{
			return static_cast<MagicType>(i);
		}
	}
	return std::nullopt;
}

bool magic::IsMaintainedSpell(MagicType type)
{
	return type == MagicType::Forest || type == MagicType::Shield || type == MagicType::PhysicalShield;
}

float magic::GetChantsRequiredToCreate(const InfoConstants& info, MagicType type)
{
	return GetMagicEffectInfo(info, type).costToCreate;
}

float magic::GetTimerWhenOneShot(const InfoConstants& info, MagicType type)
{
	return GetMagicEffectInfo(info, type).timerWhenOneShot;
}

float magic::GetTimerWhenPlayerCasting(const InfoConstants& info, MagicType type)
{
	return GetMagicEffectInfo(info, type).timerWhenPlayerCasting;
}

float magic::GetTimerWhenCreatureCasting(const InfoConstants& info, MagicType type)
{
	return GetMagicEffectInfo(info, type).timerWhenCreatureCasting;
}

float magic::GetTimerWhenComputerPlayerCasting(const InfoConstants& info, MagicType type)
{
	return GetMagicEffectInfo(info, type).timerWhenComputerPlayerCasting;
}

bool magic::IsCreatureCastFromAbove(const InfoConstants& info, MagicType type)
{
	return GetMagicInfo(info, type).isCreatureCastFromAbove == 1;
}

bool magic::IsInAggressiveRange(const InfoConstants& info, MagicType type, float distance)
{
	const auto& effect = GetMagicEffectInfo(info, type);
	return distance >= effect.agressiveRangeMin && distance <= effect.agressiveRangeMax;
}

float magic::GetTribalPower(const GMagicEffectInfo& effect, std::span<const float, k_TribeCount> tribalPower)
{
	constexpr float k_MinimumPower = 0.5f;
	constexpr float k_MaximumPower = 100.0f;

	float power = 1.0f;
	for (size_t tribe = 0; tribe < k_TribeCount; ++tribe)
	{
		if (effect.useTribalPowerMultiplier.at(tribe) != 0)
		{
			power *= tribalPower[tribe];
		}
	}
	// Written as the original compares, so that a negative product also gives the minimum
	if (power < 0.0f)
	{
		return k_MinimumPower;
	}
	if (power > k_MaximumPower)
	{
		return k_MaximumPower;
	}
	return power > k_MinimumPower ? power : k_MinimumPower;
}

std::optional<Tribe> magic::GetTribalPowerTribe(const GMagicEffectInfo& effect,
                                                std::span<const float, k_TribeCount> tribalPower)
{
	for (size_t tribe = 0; tribe < k_TribeCount; ++tribe)
	{
		if (effect.useTribalPowerMultiplier.at(tribe) != 0 && tribalPower[tribe] > 1.0f)
		{
			return static_cast<Tribe>(tribe);
		}
	}
	return std::nullopt;
}

const GSpellSeedInfo& magic::GetSpellSeedInfo(const InfoConstants& info, SpellSeedType seed)
{
	return info.spellSeed.at(static_cast<size_t>(seed));
}

int magic::GetPowerUpFromMagicType(const GSpellSeedInfo& seed, MagicType type)
{
	return GetPowerUpGesture(seed, type).level;
}

int magic::GetNumPowerUpLevels(const GSpellSeedInfo& seed)
{
	return 1 + static_cast<int>(std::ranges::count_if(seed.powerUpGestures,
	                                                  [](GestureType gesture) { return gesture != GestureType::None; }));
}

MagicType magic::GetMagicTypeFromPowerUpLevel(const GSpellSeedInfo& seed, int powerUpLevel)
{
	return seed.magicTypes.at(static_cast<size_t>(powerUpLevel - k_BasePowerUpLevel));
}

const GMagicInfo& magic::GetMagicInfoFromPowerUpLevel(const InfoConstants& info, const GSpellSeedInfo& seed, int powerUpLevel)
{
	const auto type = GetMagicTypeFromPowerUpLevel(seed, powerUpLevel);
	return GetMagicInfo(info, type != MagicType::None ? type : seed.magicTypes[0]);
}

PowerUpStep magic::GetPowerUpGesture(const GSpellSeedInfo& seed, MagicType type)
{
	if (seed.magicTypes[0] == type)
	{
		return {};
	}
	for (size_t level = 0; level < k_PowerUpLevelCount; ++level)
	{
		if (seed.magicTypes.at(level + 1) == type)
		{
			return {.gesture = seed.powerUpGestures.at(level), .level = static_cast<int>(level)};
		}
	}
	return {};
}

bool magic::SpellSeedIsOfMagicType(const GSpellSeedInfo& seed, MagicType type)
{
	return std::ranges::find(seed.magicTypes, type) != seed.magicTypes.end();
}

std::optional<SpellSeedType> magic::FindFirstSpellSeedForMagicType(const InfoConstants& info, MagicType type)
{
	return FindSeed(info, [type](const GSpellSeedInfo& seed) { return SpellSeedIsOfMagicType(seed, type); });
}

PowerUpStep magic::GetPowerUpGestureForMagicType(const InfoConstants& info, MagicType type)
{
	const auto seed = FindFirstSpellSeedForMagicType(info, type);
	return seed ? GetPowerUpGesture(GetSpellSeedInfo(info, *seed), type) : PowerUpStep {};
}

std::optional<SpellSeedType> magic::FindSpellSeedByIcon(const InfoConstants& info, uint32_t iconIndex)
{
	return FindSeed(info, [iconIndex](const GSpellSeedInfo& seed) { return seed.exists != 0 && seed.iconIndex == iconIndex; });
}

std::optional<SpellSeedType> magic::FindSpellSeedByName(const InfoConstants& info, std::string_view name)
{
	return FindSeed(info, [name](const GSpellSeedInfo& seed) { return EqualsIgnoringCase(name, seed.debugString); });
}

int magic::GetPowerUpLevel(const InfoConstants& info, MagicType type)
{
	const auto seed = FindFirstSpellSeedForMagicType(info, type);
	return seed ? GetPowerUpFromMagicType(GetSpellSeedInfo(info, *seed), type) : k_BasePowerUpLevel;
}
