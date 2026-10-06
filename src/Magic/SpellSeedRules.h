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

namespace openblack
{
struct GSpellSeedInfo;
}

// A miracle held in the hand as a seed: how the hand casts it, how charged it is and when it is ready. Pure functions of
// a seed's record and state.

namespace openblack::magic
{

/// How the hand casts a seed's miracle
enum class CastStyle : uint8_t
{
	/// Held: the miracle runs from the hand while the button is down (lightning, water, food, wood); let go, the seed
	/// keeps what prayer power is left for another go
	Held,
	/// Thrown: pressing readies it, letting go casts it at the hand's point with the hand's movement (fireball, storm,
	/// shield, the flocks)
	OnRelease,
	/// Placed: pressing casts it at the hand's point at once (heal, forest, teleport, the beam explosion)
	OnPress,
};
[[nodiscard]] CastStyle CastStyleOf(SpellCastType castType);
[[nodiscard]] CastStyle CastStyleOf(const GSpellSeedInfo& seed);

/// The prayer power a seed still needs to cast a miracle that costs so much to create
[[nodiscard]] float ChantNeeded(float costToCreate, float store);
/// How charged a seed is: its store over the cost, at most 1; fully charged for a miracle that costs nothing
[[nodiscard]] float SeedPower(float store, float costToCreate);
/// Whether a seed held for some turns is ready to cast: a seed from a worship site waits a delay, one that is ready at
/// once (from a dispenser) is always ready
[[nodiscard]] bool SeedReadyAfter(uint32_t turnsInHand, float turnSeconds, float delaySeconds);

} // namespace openblack::magic
