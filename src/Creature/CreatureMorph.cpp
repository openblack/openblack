/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureMorph.h"

#include <cmath>

#include <algorithm>

using namespace openblack;
using Appearance = creature::CreatureBody::Appearance;

namespace
{
float Axis(float value)
{
	return std::clamp(value, -1.0f, 1.0f);
}
} // namespace

creature_morph::Morph creature_morph::FromAttributes(float alignment, float fatness, float strength, float speciesStrength)
{
	const auto shownStrength = (k_SpeciesStrengthWeight * speciesStrength) + ((1.0f - k_SpeciesStrengthWeight) * strength);
	return {
	    .evilGood = Axis(alignment),
	    .thinFat = Axis((2.0f * fatness) - 1.0f),
	    .weakStrong = Axis((2.0f * shownStrength) - 1.0f),
	};
}

float creature_morph::ClampScale(float scale)
{
	return std::clamp(scale, k_MinScale, k_MaxScale);
}

float creature_morph::RestHeight(std::span<const glm::mat4> rest)
{
	float highest = 0.0f;
	float lowest = 0.0f;
	for (const auto& bone : rest)
	{
		highest = std::max(highest, bone[3].y);
		lowest = std::min(lowest, bone[3].y);
	}
	return highest - lowest;
}

float creature_morph::DrawnScale(float size, float restHeight)
{
	return restHeight > 0.0f ? size * k_HeightAtSizeOne / restHeight : size;
}

Appearance creature_morph::EvilGoodMesh(float value)
{
	return value < 0.0f ? Appearance::Evil : Appearance::Good;
}

Appearance creature_morph::ThinFatMesh(float value)
{
	return value < 0.0f ? Appearance::Thin : Appearance::Fat;
}

Appearance creature_morph::WeakStrongMesh(float value)
{
	return value < 0.0f ? Appearance::Weak : Appearance::Strong;
}

float creature_morph::EaseFatness(float shown, float fatness)
{
	return shown + std::clamp(fatness - shown, -k_MaxFatnessStep, k_MaxFatnessStep);
}

creature_morph::Refresh creature_morph::RefreshDrawn(const Morph& drawn, const Morph& target)
{
	if (std::abs(target.evilGood - drawn.evilGood) >= k_RefreshThreshold)
	{
		return {.drawn = target, .animations = true, .vertices = true};
	}
	if (std::abs(target.thinFat - drawn.thinFat) >= k_RefreshThreshold)
	{
		return {.drawn = {.evilGood = drawn.evilGood, .thinFat = target.thinFat, .weakStrong = target.weakStrong},
		        .animations = true,
		        .vertices = true};
	}
	if (std::abs(target.weakStrong - drawn.weakStrong) >= k_RefreshThreshold)
	{
		return {.drawn = {.evilGood = drawn.evilGood, .thinFat = drawn.thinFat, .weakStrong = target.weakStrong},
		        .animations = false,
		        .vertices = true};
	}
	return {.drawn = drawn, .animations = false, .vertices = false};
}
