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
#include <vector>

#include <glm/vec3.hpp>

#include "Enums.h"

// How the living take up the reactions spread round them: how urgent a reaction is to each by its kind and distance,
// how long they keep reacting and how soon they may react to the same kind again, and when a new reaction takes the
// place of the one a living thing has. Pure, tested with made-up tables.

namespace openblack::magic
{

/// The map's units are this many to a metre
inline constexpr float k_MapUnitsPerMetre = 6553.6f;
/// Within this many map units of a miracle that makes it flee, a living thing flees it more urgently the closer it is
inline constexpr float k_FleeUrgencyUnits = 600000.0f;

/// The quick distance in map units: the greater of the two ways across the land and half the lesser
[[nodiscard]] float FastMapDistance(glm::vec3 a, glm::vec3 b);

/// How urgent a kind of reaction is to a living thing before its distance counts (villager_reaction::Priority scales it
/// by that): a miracle that frightens more urgently the closer it is, by whole steps; a creature takes no notice of a
/// miracle it cast itself, frightening or nice
[[nodiscard]] uint32_t KindPriority(Reaction type, uint32_t tablePriority, float fastDistance, bool isCaster);

/// A living thing reacting to one thing changes to another (of a different kind, or for a few kinds another reaction of
/// the same kind) only when it is more urgent, and only once it has reacted for this many whole seconds: one when what
/// it reacts to now is the hand picking something up
inline constexpr uint32_t k_ReactSecondsBeforeChanging = 10;
inline constexpr uint32_t k_ReactSecondsBeforeChangingForHand = 1;
[[nodiscard]] bool ChangesReaction(Reaction current, uint32_t currentPriority, uint32_t nextPriority, uint32_t secondsReacting);

/// A villager fleeing goes this far at a time
inline constexpr float k_FleeStep = 10.0f;
/// Something whose way points within this of a villager is coming towards it, by the cosine
inline constexpr float k_ComingTowardsCosine = 0.8f;
/// Where a villager flees to from something standing still: straight away from it, a step across the land
[[nodiscard]] glm::vec3 FleePointFromStill(glm::vec3 villager, glm::vec3 object);
/// Where a villager flees to from something moving: a step across its way, to the side the villager is on, each way
/// across the land shifted by a random number up to 8 less 4
[[nodiscard]] glm::vec3 FleePointFromMoving(glm::vec3 villager, glm::vec3 object, glm::vec3 velocity, float randomX,
                                            float randomZ);
inline constexpr float k_FleeJitter = 8.0f;
/// Whether something moving with a velocity comes towards a villager
[[nodiscard]] bool ComingTowards(glm::vec3 villager, glm::vec3 object, glm::vec3 velocity);

/// Whether a kind of reaction ends once its time is up: most do, never fleeing a miracle nor one impressed by a miracle
[[nodiscard]] bool TimesOut(Reaction type);
/// Whether a reaction of a kind has outlived its time: so many turns since it was cast, or since anyone first took it up
[[nodiscard]] bool TimedOut(Reaction type, std::optional<uint32_t> stamp, uint32_t age, uint32_t turns);
/// How far a reaction reaches as it starts: from nothing, for the kinds that grow, else as far as its table says
[[nodiscard]] float StartingReach(bool grows, float maxDistance);

} // namespace openblack::magic
