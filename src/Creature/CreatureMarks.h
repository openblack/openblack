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
#include <span>
#include <vector>

#include <glm/gtc/type_precision.hpp>
#include <glm/vec2.hpp>

/// The marks fights and fire leave on a creature's skin: wounds and burns, each a small picture from an atlas of
/// damage that starts fresh and fades towards an old scar as it ages, and trails of blood, a texel at a time, darkening
/// as they dry. Marks age as the creature heals and go once they are old enough. They are painted over the skin after
/// its tattoos, the wounds first.
namespace openblack::creature_marks
{
/// A creature keeps at most this many of each kind of mark; a new one past that takes the place of the oldest
constexpr size_t k_MaxMarks = 1024;

/// A wound, burn or drop of blood on a skin
struct Mark
{
	/// Where, in texels of a skin 256 wide, and which of the base mesh's skins, 0 to 3
	uint8_t u {0};
	uint8_t v {0};
	uint8_t skin {0};
	/// How many steps of healing it has been through, 0 to 63
	uint8_t age {0};
	/// A wound's kind, 0 to 7, the row of the damage atlas it is drawn from, and its column, 0 to 7
	uint8_t type {0};
	uint8_t column {0};

	bool operator==(const Mark&) const = default;
};

/// How many steps of healing a wound of each kind lasts, which it fades from fresh to old over
constexpr std::array<uint8_t, 8> k_WoundLifetimes {32, 64, 64, 64, 64, 64, 64, 1};
/// A drop of blood goes after this many steps
constexpr uint8_t k_BloodLifetime = 63;
/// Healing comes in counts: a mark ages a step every so many
constexpr uint32_t k_CountsPerStep = 600;
/// The counts a game turn heals by, and while healing quickly
constexpr uint32_t k_CountsPerTurn = 1;
constexpr uint32_t k_FastCountsPerTurn = 50;
/// A heal effect heals by this many counts for each of its strength
constexpr uint32_t k_HealEffectCounts = 9600;

struct Marks
{
	std::vector<Mark> wounds;
	std::vector<Mark> blood;
	/// Counts of healing since the marks last aged
	uint32_t counts {0};
};

/// A mark added to a list, in place of the oldest when the list is full
void Add(std::vector<Mark>& marks, const Mark& mark);

/// What a blow or a burn cuts or scars into the skin, aimed at the groin from where it came
namespace scar
{
/// A burn marks from a point within this many times the creature's size about its groin, up and down, and a quarter of
/// it across
constexpr float k_BurnReachPerSize = 15.0f;
/// A mark is only made when where it came from is further from the groin than this, squared
constexpr float k_LeastReachSquared = 0.01f;
/// The kinds of wound a blow and a burn leave
constexpr uint8_t k_Cut = 0;
constexpr uint8_t k_DeepCut = 1;
constexpr uint8_t k_Burn = 6;

/// A blow of at least half the creature's harm cuts deep
[[nodiscard]] uint8_t BlowKind(float harm);
/// The wound's column in its row, from a draw of five: the harder the blow, the further along, 0 to 7
[[nodiscard]] uint8_t Column(uint32_t drawOfFive, float harm);
/// A burn scars as a burn only when a draw of three comes up 0, else it cuts
[[nodiscard]] uint8_t BurnKind(uint32_t drawOfThree);
/// A burn marks only when a draw of this many comes up 0: the more wounds the creature has already, the rarer
[[nodiscard]] uint32_t BurnChance(size_t wounds);
/// The texel of a skin 256 wide at a point on a triangle, s along its first side and t along its third, from its corners'
/// texture coordinates
[[nodiscard]] glm::u8vec2 TexelAt(const std::array<glm::vec2, 3>& uvs, float s, float t);
} // namespace scar

/// The marks healed by some counts: every k_CountsPerStep they age a step, and those as old as their kind lasts go.
/// Whether they aged, so the skin is to be painted again.
bool Heal(Marks& marks, uint32_t counts);

/// The damage atlases are 8 by 8 cells of 32 texels, a wound's kind picking the row and its column the column
constexpr uint32_t k_AtlasSize = 256;
constexpr uint32_t k_CellSize = 32;
constexpr uint32_t k_CellsPerRow = 8;

/// A damage atlas: colours and alpha, a row at a time, k_AtlasSize square
struct Atlas
{
	std::vector<std::array<uint8_t, 3>> colours;
	std::vector<uint8_t> alpha;
};

/// The fresh and the old look of wounds
struct DamageArt
{
	Atlas fresh;
	Atlas old;
};

/// A wound's colour and alpha at a texel of its picture: the fresh one moved towards the old by its age over its kind's
/// lifetime, in 256ths
[[nodiscard]] glm::u8vec4 WoundTexel(const DamageArt& art, const Mark& wound, uint32_t x, uint32_t y);

/// A texel of the skin, 4 bits a channel with blue the lowest, with a colour blended over it by an alpha of 255, each
/// channel blended at 8 bits and cut back to 4; opaque
[[nodiscard]] uint16_t BlendTexel(uint16_t skin, const glm::u8vec3& colour, uint8_t alpha);

/// A wound painted onto a skin of 256 by 256 texels: its picture centred on its place, cut off at the skin's edges
void PaintWound(std::span<uint16_t> skin, const DamageArt& art, const Mark& wound);

/// Blood is red when fresh, browner as it dries, and covers three quarters of the skin under it
[[nodiscard]] glm::u8vec3 BloodColour(uint8_t age);
constexpr uint8_t k_BloodAlpha = 191;
/// A drop of blood painted onto a skin of 256 by 256 texels
void PaintBlood(std::span<uint16_t> skin, const Mark& blood);
} // namespace openblack::creature_marks
