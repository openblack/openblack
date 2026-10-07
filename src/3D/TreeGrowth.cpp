/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TreeGrowth.h"

#include <algorithm>

using namespace openblack;

namespace
{
/// The rain counts in hundredths
constexpr float k_RainShare = 0.01f;
/// The land's alignment adds half of itself
constexpr float k_AlignmentShare = 0.5f;
} // namespace

float tree_growth::Growth(const Type& type, int rainOrSnow, float landAlignment)
{
	const float rain = static_cast<float>(std::max(rainOrSnow, 0)) * k_RainShare * type.rainAccelerator * type.amount;
	return (landAlignment * k_AlignmentShare + 1.0f) * (rain + type.amount);
}

float tree_growth::Grown(float size, float amount, float largest)
{
	return std::min(size + amount, largest);
}
