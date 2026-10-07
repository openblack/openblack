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

#include <functional>
#include <optional>
#include <span>

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>

#include "3D/AllMeshes.h"
#include "Enums.h"

// How the animals are turned to face their way, and the places of a flock's birds about their leader. Pure functions,
// tested on their own.

namespace openblack::animals
{

/// The angle a kind's turn angle allows in a turn, in radians: the tables count a full turn as 0x800
[[nodiscard]] float TurnAngleRadians(int turnAngle);

/// An animal's size as it is born: a young one (younger than its kind's grown up age) its kind's table's size for the
/// year before its age, grown a random way up to three quarters of the way to the size for the year after; a grown one
/// nine tenths, then a size between 0.95 and 1.05, drawn twice. `random(max)` is the game's random number up to max.
[[nodiscard]] float BirthScale(uint32_t age, uint32_t grownUpAge, std::span<const float> ageToScale,
                               const std::function<float(float)>& random);

/// How an animal's model is turned to face a heading across the land (radians from +x towards +z), tilted by a bank
/// about the way it faces. The models face -z.
[[nodiscard]] glm::mat3 Orientation(float heading, float bank);

/// A follower's place in its flock's formation, by its place in the flock counted from the leader (1 for the leader,
/// up to the number of birds for the newest): its row and column of the formation
struct FormationSlot
{
	int row;
	int column;
};
[[nodiscard]] FormationSlot FormationSlotOf(int place);
/// A formation's rows and columns are this far apart
inline constexpr float k_FormationSpacing = 10.0f;
/// Where a follower goes for its slot, in metres: the game angle of its slot's (row, column), plus the way from the
/// leader to the follower, then the row's spacing along that angle's x and the column's along its z, from the leader
[[nodiscard]] glm::vec2 FormationGoal(glm::vec2 leader, glm::vec2 follower, FormationSlot slot);

/// A killed animal lies dead this many turns, and one more, before it goes
inline constexpr int32_t k_TurnsToDieOver = 600;
/// The clip a kind of animal falls dead with, and the one it then lies dead in, once killed; none where its kind keeps
/// the clip it had
[[nodiscard]] std::optional<AnimId> DyingClip(AnimalInfo type);
[[nodiscard]] std::optional<AnimId> DeadClip(AnimalInfo type);

} // namespace openblack::animals
