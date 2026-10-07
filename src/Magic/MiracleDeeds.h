/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include "Enums.h"

/// The deeds a player's miracle shows the player's creature, which it may learn to copy: each time a miracle's effect
/// reaches something, the last thing it reached says what the player was seen doing.
namespace openblack::magic
{

/// The deeds, by their rows in the game's table of what creatures copy
inline constexpr uint32_t k_DeedCastFoodInWorshipSite = 1;
inline constexpr uint32_t k_DeedCastFoodInStoragePit = 3;
inline constexpr uint32_t k_DeedCastWoodInStoragePit = 5;
inline constexpr uint32_t k_DeedCastWoodByBuildingSite = 8;
inline constexpr uint32_t k_DeedDamageWithFire = 17;
inline constexpr uint32_t k_DeedDamageWithMagic = 18;
inline constexpr uint32_t k_DeedImpressWithMagic = 20;
inline constexpr uint32_t k_DeedCastWaterOnCrops = 33;
inline constexpr uint32_t k_DeedCastWaterToPutOutFire = 34;
inline constexpr uint32_t k_DeedHeal = 42;
/// Past the last row: nothing a creature copies
inline constexpr uint32_t k_NoDeed = 46;

/// A town of another player within this many metres of what was reached makes the deed an attempt to impress it
inline constexpr float k_DeedTownReach = 100.0f;

/// What the miracle's effect reached last
struct DeedTarget
{
	bool field {false};
	bool onFire {false};
	bool storagePit {false};
	bool worshipSite {false};
	bool buildingBeingBuilt {false};
};

/// The deed a player's miracle of a kind shows by what it reached: impressing a town of another player nearby, unless
/// the miracle is meant to anger; else burning with a fireball, damaging with lightning or a blast (not the second
/// power-up's), healing, giving food to a worship site or a storage pit, wood to a storage pit or a building going
/// up, watering crops or a fire (anything else watered counts as crops), and a flock always impresses. k_NoDeed for
/// none.
[[nodiscard]] uint32_t MiracleDeed(MagicType type, bool meantToAnger, bool anotherPlayersTownNear, const DeedTarget& target);

} // namespace openblack::magic
