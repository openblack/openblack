/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "LandForests.h"

#include "Common/GUtilsDistance.h"

using namespace openblack::ecs;

namespace
{
/// The distance a lair's score falls over, in the map's whole units
constexpr float k_LairDistanceScale = 1000.0f;
} // namespace

float land_forests::LairScore(int32_t distance)
{
	return gutils::DistanceChangeToBelief(static_cast<float>(distance), k_LairDistanceScale);
}

std::optional<size_t> land_forests::LairForest(std::span<const float> scores)
{
	std::optional<size_t> best;
	float bestScore = 0.0f;
	for (size_t i = 0; i < scores.size(); ++i)
	{
		if (!best.has_value())
		{
			best = i;
		}
		else if (scores[i] > bestScore)
		{
			bestScore = scores[i];
			best = i;
		}
	}
	return best;
}

std::optional<size_t> land_forests::Nearest(std::span<const int32_t> distances)
{
	std::optional<size_t> best;
	for (size_t i = 0; i < distances.size(); ++i)
	{
		if (!best.has_value() || distances[i] < distances[*best])
		{
			best = i;
		}
	}
	return best;
}

bool land_forests::NearTown(float distance, float reach, float wood)
{
	return distance < reach && wood != 0.0f;
}
