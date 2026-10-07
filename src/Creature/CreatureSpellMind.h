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

#include <optional>

#include "CreatureDesires.h"

// What the mood and need spells do to a creature's desires. As one starts, its desire is made fully dominant and most
// of the others are held down for a long while, so the creature wants nothing else: the hunger, poo, tiredness and
// thirst only when all are to be, and never its wish to idle or play with the player, to restore its health, make
// friends, show its state, rest, hang around at home or look around. Compassion also makes it want to make friends
// above all. The spell's desire is let go each turn while the cheat lasts, and when it runs out every desire is let go.
// As the spell wears off its desire drops below the weakest of the others. Pure, tested on their own.

namespace openblack::creature_spell_mind
{

/// The cheat lasts this many seconds, and holds the other desires down as long
inline constexpr float k_CheatSeconds = 20000.0f;
/// As a spell wears off its desire drops this far below the weakest of the others
inline constexpr float k_LeastDominantFactor = 1.3f;

/// A desire made dominant over the others for a while
struct Cheat
{
	creature_desires::Desire desire;
	/// Turns it has lasted
	uint32_t turns {0};
};

/// Whether making a desire dominant holds another down: always, never, or only when all are to be
[[nodiscard]] bool HeldDownByCheat(creature_desires::Desire other, bool all);

/// The desire is wanted above all and the others held down for the cheat's time (in turns at a rate a second); the cheat
/// to keep
[[nodiscard]] Cheat SetCheatDominant(creature_desires::Desires& desires, creature_desires::Desire desire, bool all,
                                     float turnsPerSecond);
/// A turn of a cheat: its desire is let go of again, and once its time is up every desire is; whether it still lasts
[[nodiscard]] bool StepCheat(creature_desires::Desires& desires, Cheat& cheat, float turnsPerSecond);
/// Every desire held down is let go
void ClearCheatDominance(creature_desires::Desires& desires);
/// The desire, if active, is wanted as much as it can be and every other desire as little as any can be, the species'
/// floor; it is let go of first, active or not
void MakeFullyDominantOverOthers(creature_desires::Desires& desires, creature_desires::Desire desire, float floor);
/// The desire is wanted less than any other: the weakest of the others' value over the factor, no more than its maximum
void MakeLeastDominant(creature_desires::Desires& desires, creature_desires::Desire desire,
                       float factor = k_LeastDominantFactor);

} // namespace openblack::creature_spell_mind
