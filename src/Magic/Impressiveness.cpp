/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Impressiveness.h"

#include <algorithm>

using namespace openblack::magic;

float openblack::magic::DistanceChangeToBelief(float distance, float maxDistance)
{
	// The curve's middle sits nine tenths of the reach in, so it falls steeply near the edge
	constexpr float k_Shift = 0.9f;
	constexpr float k_StepsPerUnit = 20.5f;
	const float x = maxDistance > 0.0f ? std::clamp(-distance / maxDistance, -1.0f, 1.0f) : -1.0f;
	const float y = std::clamp(x + k_Shift, -1.0f, 1.0f);
	const auto last = static_cast<int>(k_BeliefSigmoid.size()) - 1;
	const int index = std::min(static_cast<int>((y + 1.0f) * k_StepsPerUnit), last);
	return k_BeliefSigmoid.at(static_cast<size_t>(std::max(index, 0)));
}

float openblack::magic::ImpressiveValue(const ImpressionInputs& inputs)
{
	return inputs.landBalance * inputs.impressiveValue * inputs.reactionMultiplier *
	       DistanceChangeToBelief(inputs.distance, inputs.maxDistance) * inputs.power * inputs.boredom;
}

float openblack::magic::ReactionMultiplier(Reaction type, float tableMultiplier, std::optional<float> townDesire)
{
	if (type != Reaction::ReactToFood && type != Reaction::ReactToWood)
	{
		return tableMultiplier;
	}
	return townDesire.value_or(1.0f);
}

float openblack::magic::BoredomAfterImpression(float boredom, float step)
{
	return std::max(boredom + step, k_LeastBoredom);
}

float openblack::magic::BoredomAtTownTurn(float boredom, float addition, float lostTownScale)
{
	const float recovered = (lostTownScale * addition) + boredom;
	return recovered < 1.0f ? recovered : boredom;
}

float openblack::magic::ImpressionAlignment(float alignmentModifier, std::optional<float> townDesire)
{
	return townDesire.has_value() ? *townDesire * alignmentModifier : alignmentModifier;
}

float openblack::magic::FleeFromSpellPriority(float basePriority, float distance)
{
	if (distance >= k_FleeUrgencyReach)
	{
		return basePriority;
	}
	return basePriority + k_FleeUrgencyBonus * (1.0f - distance / k_FleeUrgencyReach);
}
