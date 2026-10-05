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

Appearance creature_morph::NearestMesh(const Morph& morph)
{
	constexpr float k_Halfway = 0.5f;
	const auto evilGood = std::abs(morph.evilGood);
	const auto thinFat = std::abs(morph.thinFat);
	const auto weakStrong = std::abs(morph.weakStrong);
	const auto furthest = std::max({evilGood, thinFat, weakStrong});
	if (furthest < k_Halfway)
	{
		return Appearance::Base;
	}
	if (furthest == evilGood)
	{
		return EvilGoodMesh(morph.evilGood);
	}
	if (furthest == thinFat)
	{
		return ThinFatMesh(morph.thinFat);
	}
	return WeakStrongMesh(morph.weakStrong);
}
