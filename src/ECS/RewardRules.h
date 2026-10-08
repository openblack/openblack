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
#include <functional>
#include <numbers>

#include <glm/vec3.hpp>

/// A reward chest: it falls from the sky turning as it comes, thumps on the land shaking the camera, and a cloud of dust
/// rises and fades about it
namespace openblack::ecs::reward
{

/// How high above the land a chest from the sky starts, and how fast it falls and turns
inline constexpr float k_FallFrom = 150.0f;
inline constexpr float k_FallSpeed = 50.0f;
inline constexpr float k_FallTurnsPerSecond = std::numbers::pi_v<float>;
/// Its weight, whatever its size
inline constexpr float k_Weight = 789.0f;

/// The camera shake of its landing: within this reach of a point, at this strength, for this long, along every axis
inline constexpr float k_ShakeRadius = 200.0f;
inline constexpr float k_ShakeStrength = 1.0f;
inline constexpr float k_ShakeSeconds = 0.4f;

/// The cloud of dust about it, made with it and shown only once it has fallen
inline constexpr size_t k_DustSprites = 27;
inline constexpr float k_DustMilliseconds = 2500.0f;
inline constexpr float k_DustSpread = 2.0f;
inline constexpr float k_DustRise = 2.0f;
inline constexpr float k_DustStartSize = 0.7f;
inline constexpr float k_DustGrowth = 1.8f;
inline constexpr float k_DustSmallest = 0.0001f;
inline constexpr float k_DustFadePerMillisecond = 0.0004f;
inline constexpr float k_DustLeastShade = 0.25f;
inline constexpr std::array<float, 3> k_DustColour = {212.0f, 180.0f, 140.0f};
inline constexpr float k_DustFrames = 15.0f;
inline constexpr uint32_t k_DustFrameMask = 63;

/// How high above the land a chest falling from the sky is, after so many seconds; at or below nothing it has landed
[[nodiscard]] float FallHeight(float seconds);
/// How far it has turned about its up axis
[[nodiscard]] float FallYaw(float seconds);

/// One puff of its dust as it is made: where it is about the chest, its frame on the smoke sheet and its colour
struct DustSprite
{
	glm::vec3 offset {0.0f};
	uint32_t frame {0};
	std::array<uint8_t, 3> rgb {};
};
/// The dust's puffs; random(a, b) draws a number between them, in the order the game draws them
[[nodiscard]] std::array<DustSprite, k_DustSprites> MakeDust(const std::function<float(float, float)>& random);

/// How the dust looks with so much of its time left: its alpha and its full width
struct DustLook
{
	uint8_t alpha {0};
	float size {k_DustStartSize};
};
[[nodiscard]] DustLook LookOfDust(float millisecondsLeft);

} // namespace openblack::ecs::reward
