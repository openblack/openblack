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
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"

/// The prints creatures leave as they walk: a dark paw, hoof or hand laid on the land under the lower foot at each
/// footstep, fading away over about five seconds. Only creatures leave them.
namespace openblack::creature_footprints
{
/// The most prints there are at once; new ones are dropped until old ones have faded
constexpr size_t k_Capacity = 256;
/// How opaque a fresh print is, of 255
constexpr uint8_t k_StartAlpha = 128;
/// The prints fade in steps at least this many milliseconds of game time apart
constexpr float k_FadeIntervalMs = 200.0f;
/// How much opacity, of 255, they lose per millisecond, so that a fresh print is gone in a little over five seconds
constexpr float k_FadePerMs = 0.025f;
/// How far over the land each corner of a print lies
constexpr float k_Lift = 0.15f;
/// Half the side of a print's square, for a size of 1
constexpr float k_HalfSide = 0.3f;
/// How big a print is for each unit of the creature's size, before its species' scale
constexpr float k_SizePerCreatureSize = 4.0f;
/// The prints' pictures are cells side by side along the top row of a texture, this many to the row
constexpr uint8_t k_CellsPerRow = 8;
/// The smiley face every creature leaves on the first of April
constexpr uint8_t k_SmileyCell = 7;

/// The picture a species leaves and how big it is
struct SpeciesPrint
{
	uint8_t cell;
	float scale;

	constexpr bool operator==(const SpeciesPrint&) const noexcept = default;
};

/// By the species' row in the game's creature tables (creature::InfoRow): hands for the apes, the chimp, the ogre, the
/// mandrill and the gorilla, the ape's twice as big; cat paws for the tiger, leopard, wolf and lion; cloven hooves for
/// the cow, sheep and rhino; a horseshoe for the horse; bear paws for both bears; the tortoise's foot; a round hoof for
/// the zebra; the smiley face; and the crocodile's, which the game leaves with no size at all
constexpr std::array<SpeciesPrint, 19> k_SpeciesPrints = {{
    {.cell = 0, .scale = 2.0f}, // ape
    {.cell = 2, .scale = 1.0f}, // cow
    {.cell = 4, .scale = 1.0f}, // tiger
    {.cell = 4, .scale = 1.0f}, // leopard
    {.cell = 4, .scale = 1.0f}, // wolf
    {.cell = 4, .scale = 1.0f}, // lion
    {.cell = 3, .scale = 1.0f}, // horse
    {.cell = 5, .scale = 1.0f}, // tortoise
    {.cell = 6, .scale = 1.0f}, // zebra
    {.cell = 1, .scale = 1.0f}, // brown bear
    {.cell = 1, .scale = 1.0f}, // polar bear
    {.cell = 2, .scale = 1.0f}, // sheep
    {.cell = 0, .scale = 1.0f}, // chimp
    {.cell = 0, .scale = 1.0f}, // ogre
    {.cell = 0, .scale = 1.0f}, // mandrill
    {.cell = 2, .scale = 1.0f}, // rhino
    {.cell = 0, .scale = 1.0f}, // gorilla
    {.cell = k_SmileyCell, .scale = 1.0f},
    {.cell = 0, .scale = 0.0f}, // crocodile
}};

/// The print a species leaves, the smiley face on the first of April but as big as the species' own
[[nodiscard]] SpeciesPrint PrintOf(CreatureType species, bool aprilFools);
/// Whether a day of the year, its month counted from 1, is the first of April
[[nodiscard]] constexpr bool IsAprilFools(int month, int day)
{
	return month == 4 && day == 1;
}

/// A foot as it is posed this frame, in the world
struct Foot
{
	glm::vec3 position;
	/// Its turn about the up axis, as the game reads it from the foot's matrix
	float yaw;
};
/// A foot read from its bone's world matrix: the bone's origin, and its turn about the up axis
[[nodiscard]] Foot FootOf(const glm::mat4& world);

/// Which foot a print goes under: the lower, the left only when it is lower than the right
[[nodiscard]] constexpr bool LeftIsLower(const Foot& right, const Foot& left)
{
	return left.position.y < right.position.y;
}

/// A print as laid on the land
struct Footprint
{
	std::array<glm::vec3, 4> corners;
	/// Where each corner falls on the prints' texture, the whole texture from 0 to 1
	std::array<glm::vec2, 4> uvs;
	/// How opaque the print is, of 255, drawn in black
	uint8_t alpha {k_StartAlpha};
};
/// The two triangles of a print's quad
constexpr std::array<uint16_t, 6> k_Indices = {0, 1, 3, 1, 2, 3};

/// The land's height at a point on it, by x and z
using AltitudeAt = std::function<float(float x, float z)>;

/// How big the side of a print's square is for a creature of a size leaving its species' print
[[nodiscard]] float Side(float creatureSize, const SpeciesPrint& print);

/// A print laid under a foot: a square of the print's size about the foot, its picture pointing the way the foot is
/// turned plus half a turn, each corner on the land beneath it and lifted a little. A left foot's picture is flipped
/// across its length.
[[nodiscard]] Footprint MakeFootprint(const glm::vec3& foot, float angle, float side, uint8_t cell, bool left,
                                      const AltitudeAt& altitudeAt);

/// The prints laid by every creature and still showing, the oldest first
struct Trail
{
	std::vector<Footprint> prints;
	/// The game time since they last faded
	float sinceFadeMs {0.0f};
};

/// Adds a print, unless the trail is full. Whether it was added.
bool Add(Trail& trail, const Footprint& print);
/// Fades the prints as the game time passes: once at least k_FadeIntervalMs have gone by since the last fade, each
/// print loses k_FadePerMs for every millisecond of them, and those faded out are taken away, the rest kept in order
void Fade(Trail& trail, float elapsedMs);
} // namespace openblack::creature_footprints
