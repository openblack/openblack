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

#include <glm/vec2.hpp>

// How an animal walks or flies to where it is going, a turn at a time, as the game moves it over open land: it turns
// towards its goal by at most its kind's turn angle (less when close, so it doesn't circle its goal), steps its speed
// along its heading, and once its goal is within a step it makes the last step onto it and has arrived. Positions are
// map units (6553.6 to the metre), angles game angles (2048 to the circle), speeds speed states (map units a turn).
// Pure functions, tested on their own.

namespace openblack::animals
{

/// Where a move has got to
enum class MoveStage : uint8_t
{
	/// Set up within a step of its goal: the next turn puts it there
	AtGoal,
	/// Stepping towards its goal
	StepThrough,
	/// Within a step: the next turn makes the last step
	FinalStep,
};

/// A walker's or flyer's move across the land
struct Move
{
	glm::ivec2 position {0};
	glm::ivec2 goal {0};
	/// The step of a turn, in map units
	glm::ivec2 step {0};
	uint16_t angle {0};
	uint16_t speed {0};
	MoveStage stage {MoveStage::AtGoal};
};

/// A turn towards an angle: the angle it now faces, and how far and which way it turned (0 when facing it already)
struct Turn
{
	uint16_t angle {0};
	int32_t step {0};
	int32_t direction {0};
};
/// A turn from one game angle towards another by at most the turn angle. Further off than that, an animal close to its
/// goal (nearer than twice its step over the turn angle in radians, `distance` in metres) turns harder, so that it
/// doesn't circle the goal.
[[nodiscard]] Turn TurnTowards(uint16_t angle, uint16_t target, uint16_t turnAngle, uint16_t speed, float distance);

/// The step a speed makes along a game angle in a turn
[[nodiscard]] glm::ivec2 StepAlong(uint16_t angle, uint16_t speed);
/// Whether a point is within a step of another
[[nodiscard]] bool WithinStep(glm::ivec2 position, glm::ivec2 goal, uint16_t speed);

/// A move set up towards a goal: turned towards it, and arrived already if within a step. The turn made is returned too.
[[nodiscard]] Turn SetUpMove(Move& move, glm::ivec2 goal, uint16_t turnAngle);
/// A turn of a move: whether it has arrived, and the turn it made (none while making its last step)
struct MoveResult
{
	bool arrived {false};
	Turn turn;
	bool turned {false};
};
[[nodiscard]] MoveResult StepMove(Move& move, uint16_t turnAngle);

/// A bird's height above the land after a turn's move. Flying (a goal height of 2 m and more) its height in the air
/// climbs or sinks by at most the change towards the goal's height above the land there, and stays 2 m over the land
/// under it; lower, its height above the land itself does.
[[nodiscard]] float FlyingHeight(float groundBefore, float heightBefore, float groundAfter, float groundAtGoal,
                                 float goalHeight, float change);

/// The point a hunter makes for to reach its prey: from the prey towards the hunter by both their radii, in metres
[[nodiscard]] glm::vec2 WorkingPosition(glm::vec2 prey, float preyRadius, glm::vec2 hunter, float hunterRadius);

/// Whether a point (metres) lies outside both circles an animal at a point and facing a game angle turns in at its
/// speed and turn angle: the points it could reach without circling
[[nodiscard]] bool OutsideTurningCircles(glm::vec2 animal, uint16_t angle, uint16_t speed, uint16_t turnAngle, glm::vec2 point);

/// Whether the land runs all the way under a bird's line to a point (metres): from the point back to the bird, both
/// first set on the middles of a 16 m grid, in 16 m steps across or along whichever keeps nearer the line, every map
/// cell met has its block of the land
[[nodiscard]] bool OverLandAllTheWay(glm::vec2 bird, glm::vec2 point, const std::function<bool(glm::ivec2 cell)>& hasLand);

} // namespace openblack::animals
