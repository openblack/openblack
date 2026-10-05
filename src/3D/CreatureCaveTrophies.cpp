/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureCaveTrophies.h"

#include <algorithm>
#include <functional>
#include <string_view>

#include <fmt/format.h>

namespace openblack::CreatureCaveTrophies
{

namespace
{
constexpr std::array<std::string_view, k_BeltColours> k_Belts {"WHITE", "YELLOW", "BLUE", "GREEN", "RED", "PURPLE", "BLACK"};
constexpr std::array<std::string_view, 5> k_Medals {"WOOD", "BRONZE", "SILVER", "GOLD", "GEM"};
constexpr uint32_t k_BeltIcons = k_BeltColours * k_BeltLevels;
constexpr uint32_t k_MedalsPerMetal = 5;

/// The points of the room's mesh the two rows of belts hang at, and the medals stand at, every other point
constexpr uint32_t k_FirstBeltPoint = 16;
constexpr uint32_t k_FirstMedalPoint = 6;
/// A row of belts fills by this much of the fight balance's side, five to a colour
constexpr float k_BeltFill = 34.0f;
constexpr int32_t k_LevelsPerColour = 5;
/// The creature's magic scroll divides the miracles' percentages by this many
constexpr float k_MiracleCount = 42.0f;
} // namespace

std::string IconName(uint32_t icon)
{
	if (icon < k_BeltIcons)
	{
		return fmt::format("I_BELT_{}_0{}", k_Belts.at(icon / k_BeltLevels), (icon % k_BeltLevels) + 1);
	}
	const auto medal = icon - k_BeltIcons;
	return fmt::format("I_MEDAL_{}0{}", k_Medals.at(medal / k_MedalsPerMetal), (medal % k_MedalsPerMetal) + 1);
}

MiracleLearning LearningOf(std::span<const int32_t> percents)
{
	MiracleLearning learning;
	std::vector<int32_t> sorted(percents.begin(), percents.end());
	std::ranges::sort(sorted, std::greater<> {});
	for (size_t i = 0; i < sorted.size(); ++i)
	{
		learning.overall += static_cast<float>(sorted[i]);
		if (i < learning.best.size())
		{
			learning.best.at(i) = static_cast<float>(sorted[i]);
		}
	}
	learning.overall /= k_MiracleCount;
	return learning;
}

std::vector<Trophy> Choose(float fightBalance, const MiracleLearning& learning)
{
	std::vector<Trophy> trophies;
	// Two rows of seven belts: the first fills as the balance leans one way, the second the other
	const float side = (fightBalance + 1.0f) * 0.5f;
	for (uint32_t i = 0; i < 2 * k_BeltColours; ++i)
	{
		const float fill = i < k_BeltColours ? side : 1.0f - side;
		const auto colour = i % k_BeltColours;
		const int32_t level = static_cast<int32_t>(fill * k_BeltFill) - static_cast<int32_t>(colour) * k_LevelsPerColour;
		if (level >= 1)
		{
			const auto icon =
			    (colour * k_BeltLevels) + static_cast<uint32_t>(std::min(level, static_cast<int32_t>(k_BeltLevels))) - 1;
			trophies.push_back({.point = k_FirstBeltPoint + i, .icon = icon, .medal = false, .environmentMapped = true});
		}
	}
	// A medal for all the miracles, then one for each of the best four
	for (uint32_t k = 0; k <= k_BestMiracles; ++k)
	{
		const float percent = k == 0 ? learning.overall : learning.best.at(k - 1);
		// The game takes a hundredth of it in the FPU's extended precision, where 20% is a whole five levels
		const auto level = std::min(static_cast<int32_t>(static_cast<double>(percent) * 0.01 * k_MedalLevels),
		                            static_cast<int32_t>(k_MedalLevels));
		if (level >= 1)
		{
			trophies.push_back({.point = k_FirstMedalPoint + (2 * k),
			                    .icon = k_BeltIcons + static_cast<uint32_t>(level) - 1,
			                    .medal = true,
			                    .environmentMapped = level > static_cast<int32_t>(k_MedalsPerMetal)});
		}
	}
	return trophies;
}

} // namespace openblack::CreatureCaveTrophies
