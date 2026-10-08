/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TempleHeart.h"

#include <cmath>

#include <optional>

namespace openblack::physics::temple_heart
{

Target Choose(std::span<const Town> towns)
{
	std::optional<entt::entity> first;
	for (const auto& town : towns)
	{
		for (const auto& building : town.buildings)
		{
			if (!building.available || !(building.life > 0.0f) || !(building.built > 0.0f) || building.field ||
			    building.footballPitch)
			{
				continue;
			}
			if (building.life > k_SoundLife)
			{
				return {.kind = TargetKind::Building, .entity = building.entity};
			}
			if (!first.has_value())
			{
				first = building.entity;
			}
		}
	}
	if (first.has_value())
	{
		return {.kind = TargetKind::Building, .entity = *first};
	}
	for (const auto& town : towns)
	{
		for (const auto& villager : town.homeless)
		{
			if (villager.available)
			{
				return {.kind = TargetKind::Villager, .entity = villager.entity};
			}
		}
	}
	return {};
}

float Harm(glm::vec3 velocity, float mass)
{
	const float harm =
	    std::sqrt((velocity.x * velocity.x) + (velocity.y * velocity.y) + (velocity.z * velocity.z)) * mass * k_HarmPerMomentum;
	return k_MostHarm < harm ? k_MostHarm : harm;
}

} // namespace openblack::physics::temple_heart
