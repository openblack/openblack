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

#include <entt/core/hashed_string.hpp>
#include <glm/vec2.hpp>

/// The lights a village keeps at night, the lanterns and campfires, and the hand's light: each lights the land around it
/// by stamping a small image of brightness into the land's luminosity, with the warm colours of the land's light.
namespace openblack::village_lights
{

/// The image a village light stamps, 14 cells across
inline constexpr entt::hashed_string k_VillageImageId = entt::hashed_string("raw/village_diffuse");
/// The image the hand's light stamps, 12 cells across
inline constexpr entt::hashed_string k_HandImageId = entt::hashed_string("raw/light_hand");
/// A village light's image starts this far before it along x and z
inline constexpr float k_VillageReach = 50.0f;
/// The lights flicker to a new place this often
inline constexpr float k_FlickerMilliseconds = 30.0f;
/// How far they flicker either way along x and z
inline constexpr float k_FlickerReach = 0.5f;
/// The size of a lantern's glow, and how far it flickers either way
inline constexpr float k_GlowSize = 3.0f;
inline constexpr float k_GlowFlicker = 0.1f;
/// A stamp only ever lights a cell up to this luminosity: the levels below it are the warm ones of the land's light
inline constexpr int32_t k_WarmLevels = 48;

/// Whether the land is dark enough for the lights: the whole-number mean of the land's colour (0xRRGGBB) under 120
[[nodiscard]] bool IsDark(uint32_t landColour);

/// How bright the village lights are, 0 to 255, by the hour of script time: on from 17:30 to 6:00, coming up over the hour
/// from 16:30 and going down over the hour to 7:00
[[nodiscard]] float Intensity(float scriptHour);

/// The strength a stamp is drawn with, 0 to 255, from its intensity, 0 to 1, rounded down
[[nodiscard]] int32_t Strength(float intensity);

/// The strength of the village lights' stamps, by their brightness from Intensity
[[nodiscard]] int32_t VillageStrength(float intensity);

/// Where a stamp's first texel falls on the land's cells, from a point in the world's x and z: its cell, and the
/// weights, of 255, that its image is blended with along x and z
struct Placement
{
	glm::ivec2 cell;
	glm::ivec2 weight;
};
[[nodiscard]] Placement Place(glm::vec2 xz);

/// A light's flicker timer after `milliseconds` more of game time, and whether it flickers now
struct FlickerStep
{
	float timer;
	bool flickers;
};
[[nodiscard]] FlickerStep AdvanceFlicker(float timer, float milliseconds);

/// How much of a cell's luminosity, of 256, a stamp must outshine to light it, by the green of the land's brightest light:
/// the dim edges of a stamp leave lit land as it was
[[nodiscard]] int32_t Threshold(uint8_t fullLightGreen);

} // namespace openblack::village_lights
