/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

#include <optional>
#include <span>
#include <string_view>
#include <type_traits>

#include "Enums.h"
#include "InfoConstants.h"

// Typed access to the miracle tables of info.dat: one record per magic type, split over a section per kind of
// miracle, the shared effect record of every magic type (costs, timers, ranges) and the spell seed records (which
// magic each seed casts at each power-up level, and the gesture that powers it up). Pure functions of the tables, so
// they can be tested on hand-made tables.

namespace openblack::magic
{
inline constexpr size_t k_MagicTypeCount = 42;
inline constexpr size_t k_SpellSeedCount = 30;
inline constexpr size_t k_PowerUpLevelCount = 3;
inline constexpr size_t k_TribeCount = static_cast<size_t>(Tribe::_COUNT);

/// The power-up level of a seed's plain miracle; the powered-up miracles are levels 0, 1 and 2
inline constexpr int k_BasePowerUpLevel = -1;

/// A timer value meaning the spell runs until its prayer power runs out
inline constexpr float k_NoTimeLimit = -1.0f;

static_assert(std::tuple_size_v<decltype(InfoConstants::magicEffect)> == k_MagicTypeCount);
static_assert(std::tuple_size_v<decltype(InfoConstants::spellSeed)> == k_SpellSeedCount);

/// The info.dat section that holds a magic type's record. Within the file the sections come in magic type order, so
/// the sections in this order cover magic types 0 to 41.
enum class MagicInfoSection : uint8_t
{
	General,         ///< fireball, lightning, beam explosion and their power-ups
	Heal,            ///< heal and its power-up
	Teleport,        ///< teleport
	Forest,          ///< forest
	Food,            ///< food and its power-up
	StormAndTornado, ///< storm, its power-up and the tornado
	Shield,          ///< shield and physical shield
	Wood,            ///< wood
	Water,           ///< water and its power-up
	FlockFlying,     ///< flying flock
	FlockGround,     ///< ground flock
	CreatureSpell,   ///< the sixteen spells only creatures cast

	_COUNT
};

struct MagicInfoSlot
{
	MagicInfoSection section;
	uint8_t index; ///< within the section
};

/// The section and index of a magic type's record (magic types 0 to 41)
[[nodiscard]] MagicInfoSlot SlotOf(MagicType type);

/// A magic type's record, as its common base
[[nodiscard]] const GMagicInfo& GetMagicInfo(const InfoConstants& info, MagicType type);

/// A magic type's record as its section's class, or nullptr when the magic type belongs to another section
template <class T>
[[nodiscard]] const T* GetMagicInfoAs(const InfoConstants& info, MagicType type);

/// A magic type's costs, timers and ranges
[[nodiscard]] const GMagicEffectInfo& GetMagicEffectInfo(const InfoConstants& info, MagicType type);

/// The magic type whose effect name matches, ignoring case
[[nodiscard]] std::optional<MagicType> FindMagicTypeByName(const InfoConstants& info, std::string_view name);

/// Forest, shield and physical shield are maintained: they keep drawing prayer power for as long as they last, and
/// their safety level is all of their prayer power
[[nodiscard]] bool IsMaintainedSpell(MagicType type);

/// The prayer power needed to create the miracle
[[nodiscard]] float GetChantsRequiredToCreate(const InfoConstants& info, MagicType type);

// How long the miracle lasts in seconds, by who casts it; k_NoTimeLimit when it lasts until its prayer power runs out
[[nodiscard]] float GetTimerWhenOneShot(const InfoConstants& info, MagicType type);
[[nodiscard]] float GetTimerWhenPlayerCasting(const InfoConstants& info, MagicType type);
[[nodiscard]] float GetTimerWhenCreatureCasting(const InfoConstants& info, MagicType type);
[[nodiscard]] float GetTimerWhenComputerPlayerCasting(const InfoConstants& info, MagicType type);

/// Whether a creature casts the miracle from above its target rather than from its hand
[[nodiscard]] bool IsCreatureCastFromAbove(const InfoConstants& info, MagicType type);

/// Whether a target this far away is within the range a computer player uses the miracle aggressively at
[[nodiscard]] bool IsInAggressiveRange(const InfoConstants& info, MagicType type, float distance);

/// The tribal power of a player for a miracle: the product of the player's multipliers for the tribes the miracle
/// uses, kept between 0.5 and 100. A miracle with no player behind it has a tribal power of 1.
[[nodiscard]] float GetTribalPower(const GMagicEffectInfo& effect, std::span<const float, k_TribeCount> tribalPower);

/// The first tribe the miracle uses whose multiplier is above 1
[[nodiscard]] std::optional<Tribe> GetTribalPowerTribe(const GMagicEffectInfo& effect,
                                                       std::span<const float, k_TribeCount> tribalPower);

/// A spell seed's record
[[nodiscard]] const GSpellSeedInfo& GetSpellSeedInfo(const InfoConstants& info, SpellSeedType seed);

/// The power-up level the seed casts the magic type at: k_BasePowerUpLevel for its plain miracle (and for a magic type
/// it does not cast), else 0 to 2. Like the original, an empty power-up slot matches magic type none.
[[nodiscard]] int GetPowerUpFromMagicType(const GSpellSeedInfo& seed, MagicType type);

/// How many power-up levels the seed has: its plain miracle and one for each power-up gesture
[[nodiscard]] int GetNumPowerUpLevels(const GSpellSeedInfo& seed);

/// The magic type the seed casts at a power-up level (k_BasePowerUpLevel to 2)
[[nodiscard]] MagicType GetMagicTypeFromPowerUpLevel(const GSpellSeedInfo& seed, int powerUpLevel);

/// The record of what the seed casts at a power-up level, or of its plain miracle when that level is empty
[[nodiscard]] const GMagicInfo& GetMagicInfoFromPowerUpLevel(const InfoConstants& info, const GSpellSeedInfo& seed,
                                                             int powerUpLevel);

/// The gesture that powers a seed up to a magic type, and the level it reaches
struct PowerUpStep
{
	GestureType gesture {GestureType::None};
	int level {k_BasePowerUpLevel};
};

/// How the seed powers up to the magic type. The plain miracle, and a magic type the seed does not cast, need no
/// gesture.
[[nodiscard]] PowerUpStep GetPowerUpGesture(const GSpellSeedInfo& seed, MagicType type);

/// Whether the seed casts the magic type at any level
[[nodiscard]] bool SpellSeedIsOfMagicType(const GSpellSeedInfo& seed, MagicType type);

/// The first seed that casts the magic type
[[nodiscard]] std::optional<SpellSeedType> FindFirstSpellSeedForMagicType(const InfoConstants& info, MagicType type);

/// How the first seed that casts the magic type powers up to it
[[nodiscard]] PowerUpStep GetPowerUpGestureForMagicType(const InfoConstants& info, MagicType type);

/// The first real seed shown with that icon
[[nodiscard]] std::optional<SpellSeedType> FindSpellSeedByIcon(const InfoConstants& info, uint32_t iconIndex);

/// The seed whose name matches, ignoring case
[[nodiscard]] std::optional<SpellSeedType> FindSpellSeedByName(const InfoConstants& info, std::string_view name);

/// The power-up level a miracle of this magic type runs at, from the first seed that casts it (k_BasePowerUpLevel
/// when none does). The magic records' own power-up field is unset in every row, so the seeds are the only source.
/// Not yet checked against the original.
[[nodiscard]] int GetPowerUpLevel(const InfoConstants& info, MagicType type);

namespace detail
{
[[nodiscard]] const GMagicInfo& SectionRecord(const InfoConstants& info, MagicInfoSlot slot);

/// Whether records of a section are of class T (the resource and radius classes each cover two sections)
template <class T>
[[nodiscard]] constexpr bool IsOfSection(MagicInfoSection section)
{
	using S = MagicInfoSection;
	if constexpr (std::is_same_v<T, GMagicGeneralInfo>)
	{
		return section == S::General;
	}
	else if constexpr (std::is_same_v<T, GMagicHealInfo>)
	{
		return section == S::Heal;
	}
	else if constexpr (std::is_same_v<T, GMagicTeleportInfo>)
	{
		return section == S::Teleport;
	}
	else if constexpr (std::is_same_v<T, GMagicForestInfo>)
	{
		return section == S::Forest;
	}
	else if constexpr (std::is_same_v<T, GMagicFoodInfo>)
	{
		return section == S::Food;
	}
	else if constexpr (std::is_same_v<T, GMagicWoodInfo>)
	{
		return section == S::Wood;
	}
	else if constexpr (std::is_same_v<T, GMagicResourceInfo>)
	{
		return section == S::Food || section == S::Wood;
	}
	else if constexpr (std::is_same_v<T, GMagicStormAndTornadoInfo>)
	{
		return section == S::StormAndTornado;
	}
	else if constexpr (std::is_same_v<T, GMagicShieldInfo>)
	{
		return section == S::Shield;
	}
	else if constexpr (std::is_same_v<T, GMagicRadiusSpellInfo>)
	{
		return section == S::StormAndTornado || section == S::Shield;
	}
	else if constexpr (std::is_same_v<T, GMagicWaterInfo>)
	{
		return section == S::Water;
	}
	else if constexpr (std::is_same_v<T, GMagicFlockFlyingInfo>)
	{
		return section == S::FlockFlying;
	}
	else if constexpr (std::is_same_v<T, GMagicFlockGroundInfo>)
	{
		return section == S::FlockGround;
	}
	else if constexpr (std::is_same_v<T, GMagicCreatureSpellInfo>)
	{
		return section == S::CreatureSpell;
	}
	else
	{
		static_assert(sizeof(T) == 0, "not a magic record class");
	}
}
} // namespace detail

template <class T>
const T* GetMagicInfoAs(const InfoConstants& info, MagicType type)
{
	const auto slot = SlotOf(type);
	if (!detail::IsOfSection<T>(slot.section))
	{
		return nullptr;
	}
	return static_cast<const T*>(&detail::SectionRecord(info, slot));
}
} // namespace openblack::magic
