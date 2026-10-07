/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "WaterRules.h"

#include <algorithm>

#include "Common/GUtilsDistance.h"

using namespace openblack;
using namespace openblack::magic;

namespace
{
/// The extreme water slows a full grown tree's growth as it nears this many times its size, and halves it
constexpr float k_MostScale = 3.0f;
constexpr float k_FullGrownShare = 0.5f;
} // namespace

void magic::WaterField(WaterCrop& crop, const WaterCropType& type)
{
	if (static_cast<float>(crop.timesSown) <= type.timesToSow)
	{
		// Sown at once: one more time than it needs
		crop.timesSown = static_cast<uint8_t>(type.timesToSow + 1.0f);
		return;
	}
	if (crop.age <= type.ageRipe)
	{
		crop.age += type.effectOfWater;
		crop.food += type.effectOfWater * type.totalFood / type.ageRipe;
	}
}

WaterTreeResult magic::WaterTree(float scale, float target, const WaterTreeType& type, bool extreme)
{
	WaterTreeResult result {.scale = scale, .target = target, .canGrow = scale < target};
	if (!result.canGrow && !extreme)
	{
		return result;
	}
	float amount = type.growthAmount * type.waterAccelerator;
	if (!result.canGrow)
	{
		amount *= gutils::GetDistanceModifier(scale, k_MostScale) * k_FullGrownShare;
	}
	if (extreme)
	{
		result.target = std::max(result.target, scale + amount);
	}
	if (scale != result.target)
	{
		result.scale = std::min(scale + amount, result.target);
	}
	return result;
}
