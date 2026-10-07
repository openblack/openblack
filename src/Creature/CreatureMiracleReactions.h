/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <vector>

#include <glm/vec2.hpp>

#include "CreatureIdleMind.h"

// What a creature does about a miracle near it. Frightened by a nasty one, it may start back in fright, then runs away
// from where it struck. Curious about one (a nice one of its own player's, or a nasty one when it isn't afraid or is on
// the learning leash), it may first turn to it and point at it, then goes up to it, turns to face it and points at it,
// or waits there puzzled. Pure agendas, tested on their own.

namespace openblack::creature_mind
{

/// The creature arrives this many times its height from where a miracle struck
inline constexpr float k_MiracleApproachHeights = 5.0f;
/// It waits this long puzzled at a miracle it went to look at, in seconds
inline constexpr float k_PuzzledSeconds = 2.1f;
/// The frightened start it may give
inline constexpr size_t k_FrightenedAnimation = 61;

/// Running away from where a frightening miracle struck, the start of fright one time in two
[[nodiscard]] std::vector<Step> RunAwayFromMiracle(glm::vec2 point, float height, const Random& random);
/// Going to look at where a miracle struck
[[nodiscard]] std::vector<Step> ExamineMiracle(glm::vec2 point, float height, const Random& random);

} // namespace openblack::creature_mind
