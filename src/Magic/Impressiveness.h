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

#include <array>
#include <optional>

#include "Enums.h"

// How much a miracle impresses whoever watches it, and how urgently they react. A miracle that impresses gives belief in
// its caster to the people who see it and impresses creatures; closer watchers are impressed more, along a steep S
// curve of the distance over the reaction's reach, and a town that has seen the same kind of miracle again and again is
// impressed less each time. Pure rules, tested with made-up values.

namespace openblack::magic
{

/// The S curve the distance runs through: 41 steps from nothing to all
inline constexpr std::array<float, 41> k_BeliefSigmoid {
    0.0f,        3.6e-9f,     1e-8f,        2.8e-8f,      7.78e-8f,     2.163e-7f,   6.018e-7f,   1.674e-6f,   4.6568e-6f,
    1.29542e-5f, 3.60351e-5f, 1.002359e-4f, 2.787865e-4f, 7.751431e-4f, 0.00215332f, 0.00596721f, 0.01642494f, 0.04439174f,
    0.11443728f, 0.26442435f, 0.5f,         0.73557568f,  0.88556272f,  0.95560825f, 0.98357505f, 0.9940328f,  0.99784666f,
    0.99922484f, 0.99972123f, 0.99989974f,  0.99996394f,  0.99998707f,  0.99999535f, 0.99999833f, 0.9999994f,  0.99999976f,
    0.99999994f, 1.0f,        1.0f,         1.0f,         1.0f};

/// How much of an impression reaches a watcher at a distance from the miracle, given how far the reaction reaches: all
/// of it close by, a little over a tenth at the edge, and no less beyond
[[nodiscard]] float DistanceChangeToBelief(float distance, float maxDistance);

/// What goes into how much a miracle impresses one watcher
struct ImpressionInputs
{
	/// The land's balance of impressiveness, which scripts raise on later lands
	float landBalance {1.0f};
	/// The miracle's own impressiveness, from its effect's table
	float impressiveValue {0.0f};
	/// The reaction's multiplier
	float reactionMultiplier {1.0f};
	float distance {0.0f};
	float maxDistance {1.0f};
	/// The miracle's power; a plain miracle's is 1
	float power {1.0f};
	/// The watcher's town's weariness of this kind of reaction, 1 when it hasn't seen it before
	float boredom {1.0f};
};

/// How much a miracle impresses one watcher
[[nodiscard]] float ImpressiveValue(const ImpressionInputs& inputs);

/// The multiplier a reaction impresses one watcher by. A reaction to food or wood impresses by how much the watcher's
/// town wants what its table names (its want, plus the boosts the game and the scripts give it), or by exactly 1 when
/// the watcher has no town or the table names nothing. Every other reaction impresses by its table's multiplier.
[[nodiscard]] float ReactionMultiplier(Reaction type, float tableMultiplier, std::optional<float> townDesire);

/// A creature watching its own player's miracle is impressed this many times over; another's, this many
inline constexpr float k_CreatureImpressedByOwnPlayer = 4.0f;
inline constexpr float k_CreatureImpressedByOtherCreature = 12.0f;

/// A town's boredom with a kind of reaction, which starts at 1 and multiplies how impressive the next of the kind is,
/// never goes below this
inline constexpr float k_LeastBoredom = 0.0f;
/// A town's boredom with a kind of reaction after one more impression of it: the step (the belief table's boredom, which
/// is negative, times the villager's share of its town) is added
[[nodiscard]] float BoredomAfterImpression(float boredom, float step);
/// At each of a town's turns its boredom with a kind of reaction wears off by the kind's own amount, times the land's
/// lost-town scale, as long as that leaves it below 1
[[nodiscard]] float BoredomAtTownTurn(float boredom, float addition, float lostTownScale);
/// How much an impression moves the alignment of the player whose reaction it is, before it is damped: the kind's own
/// amount, times how much the town wants what it shows when the kind looks to a desire
[[nodiscard]] float ImpressionAlignment(float alignmentModifier, std::optional<float> townDesire);

/// How urgently a living thing flees a miracle at a distance: from its reaction's priority close by up by a hundred,
/// falling to the plain priority at the edge of its reach and beyond
inline constexpr float k_FleeUrgencyReach = 91.5f;
inline constexpr float k_FleeUrgencyBonus = 100.0f;
[[nodiscard]] float FleeFromSpellPriority(float basePriority, float distance);

} // namespace openblack::magic
