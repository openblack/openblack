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
#include <glm/vec3.hpp>

#include "ECS/Components/ChimneySmoke.h"

/// How the smoke of the homes' chimneys rises, drifts, spins and fades, and how each puff looks
namespace openblack::chimney_smoke
{

/// Draws a number between two, as the game's random numbers for the smoke
using Random = std::function<float(float, float)>;

/// The puffs' grey and white smoke
inline constexpr uint32_t k_HomeSmoke = 0xFFFFFF;
inline constexpr uint32_t k_WorkshopSmoke = 0x808080;

/// The air the hand stirs as it moves: where it is, which way the air goes and how fast
struct HandWind
{
	glm::vec3 position {-10000.0f, 0.0f, 0.0f};
	glm::vec3 wind {1.0f, 0.0f, 0.0f};
	float speed {0.0f};
};

/// A new smoke at a chimney: its puffs start unseen, at ages spread through their lives
[[nodiscard]] ecs::components::ChimneySmoke Create(const glm::vec3& chimney, uint32_t rgb, const Random& random);

/// The hand's velocity after a turn in which it moved by `moved`, easing towards that speed
[[nodiscard]] glm::vec3 EaseHandVelocity(const glm::vec3& velocity, const glm::vec3& moved, float millisecondsPerTurn);
/// The air the hand stirs at a position, moving at a velocity
[[nodiscard]] HandWind WindOf(const glm::vec3& position, const glm::vec3& velocity);
/// The frame's push on a smoke: the hand's air if it passes close and quickly, else a random breath across the ground
[[nodiscard]] glm::vec3 Drift(const glm::vec3& chimney, const HandWind& hand, const Random& random);

/// Whether a smoke is lit, its home having someone in. Returns whether there is anything left of it to draw.
bool UpdateState(ecs::components::ChimneySmoke& smoke, bool lit);
/// Moves the puffs on by some milliseconds of game time, pushed by the frame's drift
void Advance(ecs::components::ChimneySmoke& smoke, float milliseconds, const glm::vec3& drift);

/// How a puff looks: the frame of the smoke texture it shows, its half width and its colour with its opacity
struct Look
{
	int frame;
	float halfWidth;
	uint32_t argb;
};
[[nodiscard]] Look LookOf(int32_t age, uint32_t rgb);

} // namespace openblack::chimney_smoke
