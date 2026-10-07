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

#include "ECS/Components/PrayerPower.h"
#include "Enums.h"

// What a player's miracles cost in prayer power. A seed summoned from worship is charged with its miracle's cost to
// create before it reaches the hand; a seed from a one-shot bubble comes charged for nothing. While a miracle runs, the
// player's worship tops it up to its safety level each turn, so a maintained miracle lives as long as the player keeps
// paying. A seed dropped from the hand gives back what it still holds: its whole charge if it was never cast, or what its
// held miracle had left. Pure rules on the player's store.

namespace openblack::magic
{

/// Draws prayer power from a player's store: what was asked, or what is left when that is less. An infinite store gives
/// all that is asked. Nothing for a negative amount.
float DrawPrayer(ecs::components::PrayerPower& store, float amount);
/// Prayer power goes back into the store
void ReturnPrayer(ecs::components::PrayerPower& store, float amount);
/// Charges a seed from the store for a miracle's cost to create: what it was charged, all of the cost or what the store
/// had
float ChargeSeed(ecs::components::PrayerPower& store, float costToCreate);

/// Where a seed in the hand came from
enum class SeedOrigin : uint8_t
{
	/// Summoned from the player's worship: charged from the player's prayer power, and not ready at once
	Worship,
	/// Taken from a one-shot bubble or given by a dispenser: charged already, and ready at once
	Bubble,
};

/// Whether a seed is ready the moment it reaches the hand
[[nodiscard]] constexpr bool ReadyAtOnce(SeedOrigin origin)
{
	return origin == SeedOrigin::Bubble;
}

/// What a seed dropped from the hand gives back to the worship site of its icon: what its held miracle had left when let
/// go, or its whole charge if it was never cast. A seed made without an icon gives back nothing, having nowhere to.
[[nodiscard]] float SeedRefund(bool hasIcon, float chantStore, float storedChants, bool hasCast);

/// What a player's caster gives a miracle that asks for prayer power: the neutral player, whose miracles scripts cast,
/// gives all; another player gives from their store when they have one, and nothing without
[[nodiscard]] float PlayerMaintainSpell(PlayerNames player, ecs::components::PrayerPower* store, float amount);

} // namespace openblack::magic
