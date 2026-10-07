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

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

// The rules of the flock miracles, the flying flock of doves or bats and the ground flock of wolves: how many animals a
// cast makes, where along the hand's sweep each appears, where it heads, how big it is, how it fades out, and the
// corridor a wolf hunts along. Pure functions, tested on their own.

namespace openblack::magic::flock
{

/// Animals made each second of the sweep after the cast
inline constexpr float k_EmitPerSecond = 12.0f;
/// The fan of destinations: the last animal heads this many radians either side of the way the camera looks
inline constexpr float k_AngleVariation = 2.0f;
/// Each animal appears up to this far either side of its point along the sweep, across and along
inline constexpr float k_SpawnJitter = 0.1f;
/// A destination off the map is brought half way back, until the distance left to try is no more than this
inline constexpr float k_MinTravel = 10.0f;
/// The land's cells are this wide
inline constexpr float k_CellSize = 10.0f;
/// A direction shorter than this is no direction
inline constexpr float k_TinyDirection = 0.0001f;
/// An animal released this far (as the cross product of unit vectors) to one side of the camera's view heads that side;
/// nearer the middle, the animals alternate sides
inline constexpr float k_SideThreshold = 0.1f;
/// Doves and bats are made between these times their size, wolves between the next two
inline constexpr float k_BirdScaleMin = 2.8f;
inline constexpr float k_BirdScaleMax = 3.0f;
inline constexpr float k_WolfScaleMin = 1.5f;
inline constexpr float k_WolfScaleMax = 2.0f;
/// A spell animal fades out over this many turns once its miracle is over, or it has arrived
inline constexpr int k_FadeTurns = 20;
inline constexpr float k_FullAlpha = 255.0f;

/// How many animals a cast makes: the tables' number times the caster's tribal power, rounded to the nearest, a half
/// to the even
[[nodiscard]] int NumberToCreate(uint32_t number, float tribalPower);
/// The flying flock is bats for a caster more evil than the switch, doves otherwise
[[nodiscard]] bool IsEvil(float alignment, float alignmentSwitch);
/// How many animals should have been made by the end of a turn
[[nodiscard]] float EmitAfterTurn(float emitted, float turnSeconds);
/// How far along the turn's piece of the sweep the animal of a number appears, 0 at its start and 1 at its end
[[nodiscard]] float SpawnFraction(int created, float emittedBefore, float emittedAfter);
/// The point of the sweep at a fraction of the way from one end to the other
[[nodiscard]] glm::vec2 SpawnPoint(glm::vec2 from, glm::vec2 to, float fraction);
/// A point moved across and along by random numbers between 0 and twice the jitter each
[[nodiscard]] glm::vec2 Jitter(glm::vec2 point, float randomX, float randomZ);

/// The way the animals head, across the land: where a human player's camera looks, or from the hand to the cast point
/// for any other caster; east when that is no way at all
[[nodiscard]] glm::vec2 Direction(bool humanCaster, glm::vec3 cameraForward, glm::vec3 castPosition, glm::vec3 handPosition);
/// Which side of the view an animal heads to: the side of the camera's view it was released on, for a human caster,
/// and otherwise every other one to each side
[[nodiscard]] float Side(bool humanCaster, glm::vec2 direction, glm::vec2 spawn, glm::vec2 castPoint, int created);
/// The angle the animal of a number turns away from the direction: further for each one made
[[nodiscard]] float FanAngle(int created, float side, int numberToCreate);
/// A direction turned about the vertical
[[nodiscard]] glm::vec2 Rotate(glm::vec2 direction, float angle);
/// A point moved by whole cells of the land: each cell number moves by the travel, cut to a whole cell, and the point
/// keeps its place within its cell
[[nodiscard]] glm::vec2 StepByCells(glm::vec2 from, glm::vec2 travel);
/// Where an animal heads: the distance along the direction from where it appears, by whole cells, brought half way back
/// while off the map, the last try at the last distance over 10 m before halving; none when nowhere near enough is on
/// the map
[[nodiscard]] std::optional<glm::vec2> Destination(glm::vec2 spawn, glm::vec2 direction, float distance,
                                                   const std::function<bool(glm::vec2)>& onMap);

/// An animal's size: the low end of its kind's range plus a random number up to the range, whatever its size as it was
/// born
[[nodiscard]] float SpawnScale(bool wolf, float random);

/// The strip of land a wolf hunts along, from where it appears to where it is sent: the strip's unit normal across it,
/// where the line runs, and how far either side of it the strip reaches
struct Corridor
{
	glm::vec2 normal {1.0f, 0.0f};
	float offset {0.0f};
	float halfWidth {0.0f};
	/// The way along the strip
	glm::vec2 along {0.0f, 1.0f};
};
[[nodiscard]] Corridor MakeCorridor(glm::vec2 start, glm::vec2 destination, float halfWidth);
/// Whether a point is inside the strip and no further behind the corner of the wolf's cell than the strip is wide; a strip
/// from a point to itself runs north, across east
[[nodiscard]] bool IsOnCorridor(const Corridor& corridor, glm::vec2 point, glm::vec2 wolf);
/// Once a wolf remembers where it found prey, other prey is taken only when this holds: positions in map units (65536 to
/// a cell)
[[nodiscard]] bool CloserThanRemembered(glm::ivec2 wolf, glm::ivec2 prey, glm::ivec2 remembered);

/// A wolf this close to where it is sent has arrived, and fades away
inline constexpr float k_WolfArrival = 30.0f;
/// A wolf runs this much faster than its kind's speed, times its size
inline constexpr float k_WolfRunFactor = 1.1f;
[[nodiscard]] bool WolfArrived(glm::vec2 wolf, glm::vec2 destination);

/// What a hunter knows about something it might hunt
struct PreyFacts
{
	bool isVillager {false};
	/// An animal of another kind than the hunter
	bool isOtherAnimal {false};
	bool uneatable {false};
	/// Its height above the land
	float heightAboveLand {0.0f};
	bool hasMeat {true};
	bool skeleton {false};
	/// Dying, dead, downed or being eaten
	bool helpless {false};
	bool onCorridor {true};
	/// The hunter is fading away
	bool hunterFading {false};
};
/// Nothing more than this above the land is hunted
inline constexpr float k_PreyMaxHeight = 2.0f;
/// Whether a wolf hunts it
[[nodiscard]] bool IsPrey(const PreyFacts& prey);
/// A wolf this close to its prey brings it down, leaving it this much life
inline constexpr float k_PounceReach = 1.0f;
inline constexpr float k_DownedLife = 0.05f;
/// A villager brought down is eaten over this many turns, then dies
inline constexpr int k_BeingEatenTurns = 300;
/// The turns a clip of so many milliseconds takes to play out, waiting a whole turn at a time
[[nodiscard]] int TurnsToPlay(uint32_t playTime, uint32_t turnMilliseconds);
/// Where a wolf goes to eat its prey: its scale short of the prey, the way from the wolf in three dimensions
[[nodiscard]] glm::vec3 EatingPosition(glm::vec3 prey, glm::vec3 wolf, float scale);

} // namespace openblack::magic::flock
