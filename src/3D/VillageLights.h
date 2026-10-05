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

#include <entt/core/hashed_string.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

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

/// A light's two flames and its glow: sprites in the light's colour, their alpha half the lights' brightness. The flames
/// play the fire's cells backwards, all in time with one clock; the glow is a cell of the smoke.
inline constexpr size_t k_Flames = 2;
inline constexpr size_t k_Sprites = k_Flames + 1;
inline constexpr glm::vec3 k_SpriteColour {0xF3 / 255.0f, 0x84 / 255.0f, 0x21 / 255.0f};
inline constexpr entt::hashed_string k_FlameTextureId = entt::hashed_string("raw/S_Fire");
inline constexpr entt::hashed_string k_FlameAlphaTextureId = entt::hashed_string("raw/S_Firea");
inline constexpr entt::hashed_string k_GlowTextureId = entt::hashed_string("raw/smoke");
inline constexpr entt::hashed_string k_GlowAlphaTextureId = entt::hashed_string("raw/smokea");
/// The textures are 8 by 8 cells
inline constexpr float k_SpriteCell = 1.0f / 8.0f;
inline constexpr uint8_t k_GlowCell = 56;
/// The flames' size, and how far it varies either way
inline constexpr float k_FlameSize = 1.0f;
inline constexpr float k_FlameSizeVariation = 0.1f;
/// The smallest a flame or glow is ever made
inline constexpr float k_SmallestSprite = 1e-4f;
/// How high the sprites are above a town lantern and a country lantern's campfire
inline constexpr float k_TownSpriteHeight = 5.0f;
inline constexpr float k_CountrySpriteHeight = 1.0f;
/// The flames' cells loop every 700 ms of game time, 31 cells of the fire's 32
inline constexpr int32_t k_FlameLoopMilliseconds = 700;
inline constexpr int32_t k_FlameCells = 31;
/// Where each sprite starts in the flames' loop, as the game has it before any light is made: every new light sets all
/// three at random, and all the lights then share them
inline constexpr std::array<int32_t, k_Sprites> k_FlameStarts = {0, 13, 0};

/// The flames' clock after `milliseconds` more of game time, and how far along the loop it is, 0 to 30
struct FlameStep
{
	int32_t clock;
	int32_t step;
};
[[nodiscard]] FlameStep AdvanceFlames(int32_t clock, int32_t milliseconds);

/// The fire's cell a flame shows, by how far along the loop the clock is and where the flame starts in it
[[nodiscard]] uint8_t FlameCell(int32_t step, size_t flame, int32_t start);

/// The top left of a cell of an 8 by 8 sprite texture
[[nodiscard]] glm::vec2 SpriteCellUv(uint8_t cell);

/// How much of a cell's luminosity, of 256, a stamp must outshine to light it, by the green of the land's brightest light:
/// the dim edges of a stamp leave lit land as it was
[[nodiscard]] int32_t Threshold(uint8_t fullLightGreen);

} // namespace openblack::village_lights
