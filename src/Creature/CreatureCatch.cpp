/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureCatch.h"

#include <cmath>

#include <algorithm>
#include <limits>

#include <glm/geometric.hpp>

#include "CreatureLayers.h"

namespace openblack::creature_catch
{

bool Reaches(const Approach& approach)
{
	const glm::vec2 velocity {approach.velocity.x, approach.velocity.z};
	const float speedSquared = glm::dot(velocity, velocity);
	if (speedSquared < k_LeastSpeedSquared)
	{
		return false;
	}
	const glm::vec2 offset {approach.thing.x - approach.creature.x, approach.thing.z - approach.creature.z};
	// When it passes closest across the land
	const float seconds = -glm::dot(offset, velocity) / speedSquared;
	if (seconds > k_MostSeconds)
	{
		return false;
	}
	// The creature's animations play faster the smaller it is
	const float rate = creature_layers::PlaybackRate(approach.size);
	const float spare = seconds - approach.catchMs / (1000.0f * rate);
	if (spare < k_LeastSpare)
	{
		return false;
	}
	const float step = approach.stepMs / (1000.0f * rate);
	const float miss = glm::length(offset + velocity * seconds);
	const float reach = (0.5f * step + spare) * (-approach.stepTravel * approach.modelScale / step);
	return miss < reach;
}

Blend Weigh(glm::vec3 thing, const std::array<glm::vec3, 4>& hands, float modelScale, bool mirrored)
{
	// The hand at the moment it closes: low and to one side, low and to the other, high and to one side, high and to the
	// other
	const auto& lowSide = hands[1];
	const auto& highSide = hands[2];
	const auto& highOther = hands[3];
	float height = (thing.y - modelScale * lowSide.y) / (modelScale * highOther.y - modelScale * lowSide.y);
	const float across = mirrored ? thing.x : -thing.x;
	float side = (across + modelScale * highSide.x) / (modelScale * highSide.x - modelScale * highOther.x);
	const auto keep = [](float& weight) {
		const float kept = std::clamp(weight, k_LeastWeight, k_MostWeight);
		const bool clamped = kept != weight;
		weight = kept;
		return clamped;
	};
	const bool clamped = keep(height) | keep(side);
	return {.weights = {(1.0f - height) * (1.0f - side), (1.0f - height) * side, height * (1.0f - side), height * side},
	        .clamped = clamped};
}

Ready ReadyToCatch(glm::vec3 thing, glm::vec3 velocity, const std::array<glm::vec3, 4>& hands, float modelScale,
                   float leadSeconds)
{
	// It must be in front of the creature
	const float ahead = -thing.z;
	if (ahead < 0.0f)
	{
		return {.readiness = Readiness::GiveUp, .mirrored = false};
	}
	const float side = -thing.x;
	const auto& highSide = hands[2];
	const auto& highOther = hands[3];
	const float reach = modelScale * (highSide.x + k_ReachBeyondHand * (highOther.x - highSide.x));
	if (std::abs(side) > std::abs(reach))
	{
		return {.readiness = Readiness::Step, .mirrored = side <= 0.0f};
	}
	// When it arrives, coming at the creature along its front
	const float closing = velocity.z;
	const float arrives = closing != 0.0f ? ahead / closing : std::numeric_limits<float>::infinity();
	if (arrives >= leadSeconds + k_LateArrival)
	{
		return {.readiness = Readiness::Wait, .mirrored = false};
	}
	if (arrives > leadSeconds - k_EarlyArrival)
	{
		return {.readiness = Readiness::Catch, .mirrored = side < 0.0f};
	}
	return {.readiness = Readiness::GiveUp, .mirrored = false};
}
} // namespace openblack::creature_catch
