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
#include <span>
#include <string>
#include <vector>

namespace openblack
{

/// The belts the creature's room hangs on its attack dummies for how the creature fights, and the medals on its magic
/// plinths for how well it has learnt its miracles (CreatureRoom::UpdateBeltsAndMedals)
namespace CreatureCaveTrophies
{

/// The icons fn_00787340 loads from data/citadel/icons (0xC26F38): four of each of the seven belts, white to black,
/// then five of each of the five medals, wood to gem
constexpr size_t k_IconCount = 53;
constexpr uint32_t k_BeltColours = 7;
constexpr uint32_t k_BeltLevels = 4;
constexpr uint32_t k_MedalLevels = 25;
/// The belts' and medals' colours (their objects' +0x1B0)
constexpr uint32_t k_BeltColour = 0xFFA0A0A0;
constexpr uint32_t k_MedalColour = 0xFF808080;
/// The medals for the miracles best learnt, after the one for all of them
constexpr size_t k_BestMiracles = 4;

/// The name of an icon's mesh, without its extension
[[nodiscard]] std::string IconName(uint32_t icon);

/// An icon shown at a point of the creature's room's mesh
struct Trophy
{
	uint32_t point;
	uint32_t icon;
	bool medal;
	/// Drawn with an environment map added: every belt and every medal past wood (the object's +0xD8)
	bool environmentMapped;
};

/// How well the creature has learnt its miracles, from 0 to 100: all of them together, and the best four, best first
struct MiracleLearning
{
	float overall {0.0f};
	std::array<float, k_BestMiracles> best {};
};

/// CreatureRoom::MakeMagicScrollText: all the miracles' percentages learnt together over 42, and the best four
[[nodiscard]] MiracleLearning LearningOf(std::span<const int32_t> percents);

/// The belts and medals shown. Fight balance is from -1 to 1 (the creature's mind +0x17D04): a row of belts each way
/// fills, white first, five a colour, as it leans to its side.
[[nodiscard]] std::vector<Trophy> Choose(float fightBalance, const MiracleLearning& learning);

} // namespace CreatureCaveTrophies

} // namespace openblack
