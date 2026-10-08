/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

#include <array>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// A creature catching a thing flying at it: whether it can reach where the thing will pass in time, and the four catching
/// animations blended by how high and how far to the side the thing comes, played together to the moment the hand closes
namespace openblack::creature_catch
{
/// The step towards a catch, whose length and travel give how far a creature can reach
constexpr size_t k_CatchStep = 225;
/// The four catching animations, blended by height and side: low and to one side, low and to the other, high and to
/// one side, high and to the other
constexpr std::array<size_t, 4> k_CatchAnimations {231, 230, 229, 228};

/// A creature only catches what weighs less than this share of its own weight
constexpr float k_MostWeightShare = 0.8f;
/// A creature tries to catch another player's throw only when a draw of a hundred is at most this
constexpr uint32_t k_OtherPlayersChance = 2;
/// The thing must fly across the land at least this fast, squared
constexpr float k_LeastSpeedSquared = 1.0f;
/// It must pass closest within this many seconds, and with at least this long to spare once the hand is to close
constexpr float k_MostSeconds = 5.0f;
constexpr float k_LeastSpare = 0.05f;
/// The creature turns to face the thing first when it is more than this far round, in radians
constexpr float k_TurnAngle = 0.2618f;
/// How far the blend may lean past the four animations' own reaches before the catch misses
constexpr float k_LeastWeight = -0.2f;
constexpr float k_MostWeight = 1.2f;

/// What the catch's reach depends on
struct Approach
{
	/// Where the thing is and how it flies, and where the creature stands
	glm::vec3 thing;
	glm::vec3 velocity;
	glm::vec3 creature;
	/// The creature's size, and the scale its model is drawn at
	float size;
	float modelScale;
	/// The moment the catching hand closes, in milliseconds into the catch
	float catchMs;
	/// How long the step towards a catch lasts, in milliseconds, and how far it carries the creature across in its model's
	/// units
	float stepMs;
	float stepTravel;
};
/// Whether the creature can step to where the thing passes closest across the land before its hand must close: the
/// thing flying across fast enough, passing within five seconds with time to spare, and nearer the creature then than
/// the step reaches
[[nodiscard]] bool Reaches(const Approach& approach);

/// How the four animations are blended: how high and how far to the side the thing is between where the hand gets to in
/// each, each kept within the limits, and whether either had to be
struct Blend
{
	std::array<float, 4> weights;
	bool clamped;
};
/// The thing's place against the creature, in its own axes at the world's scale; the hand's place in each catching
/// animation at the moment it closes, in the model's units; the model's scale
[[nodiscard]] Blend Weigh(glm::vec3 thing, const std::array<glm::vec3, 4>& hands, float modelScale, bool mirrored);
} // namespace openblack::creature_catch
