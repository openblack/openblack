/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ReactionRules.h"

#include <cmath>

#include <algorithm>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

using namespace openblack::magic;

float openblack::magic::FastMapDistance(glm::vec3 a, glm::vec3 b)
{
	const float dx = std::abs(a.x - b.x) * k_MapUnitsPerMetre;
	const float dz = std::abs(a.z - b.z) * k_MapUnitsPerMetre;
	return std::max(dx, dz) + std::min(dx, dz) * 0.5f;
}

uint32_t openblack::magic::KindPriority(Reaction type, uint32_t tablePriority, float fastDistance, bool isCaster)
{
	switch (type)
	{
	case Reaction::FleeFromSpell:
		// A creature never runs from a miracle it cast itself
		if (isCaster)
		{
			return 0;
		}
		if (fastDistance < k_FleeUrgencyUnits)
		{
			return tablePriority + static_cast<uint32_t>(100.0f * (k_FleeUrgencyUnits - fastDistance) / k_FleeUrgencyUnits);
		}
		return tablePriority;
	case Reaction::LookAtNiceSpell:
	case Reaction::ReactToImpressiveSpell:
		return isCaster ? 0 : tablePriority;
	default:
		return tablePriority;
	}
}

bool openblack::magic::ChangesReaction(Reaction current, uint32_t currentPriority, uint32_t nextPriority,
                                       uint32_t secondsReacting)
{
	if (!(currentPriority < nextPriority))
	{
		return false;
	}
	const auto least =
	    current == Reaction::ReactToHandPickUp ? k_ReactSecondsBeforeChangingForHand : k_ReactSecondsBeforeChanging;
	return secondsReacting >= least;
}

glm::vec3 openblack::magic::FleePointFromStill(glm::vec3 villager, glm::vec3 object)
{
	glm::vec2 away(villager.x - object.x, villager.z - object.z);
	if (away != glm::vec2(0.0f))
	{
		away = glm::normalize(away) * k_FleeStep;
	}
	return {villager.x + away.x, villager.y, villager.z + away.y};
}

glm::vec3 openblack::magic::FleePointFromMoving(glm::vec3 villager, glm::vec3 object, glm::vec3 velocity, float randomX,
                                                float randomZ)
{
	// Across its way, which is the side of it the villager is on
	glm::vec3 across(-velocity.z, 0.0f, velocity.x);
	if (across != glm::vec3(0.0f))
	{
		across = glm::normalize(across);
	}
	const float x = across.x * k_FleeStep + randomX - k_FleeJitter * 0.5f;
	const float z = across.z * k_FleeStep + randomZ - k_FleeJitter * 0.5f;
	const float side = across.x * (villager.x - object.x) + across.z * (villager.z - object.z);
	return side < 0.0f ? glm::vec3(villager.x - x, villager.y, villager.z - z)
	                   : glm::vec3(villager.x + x, villager.y, villager.z + z);
}

bool openblack::magic::ComingTowards(glm::vec3 villager, glm::vec3 object, glm::vec3 velocity)
{
	const auto toVillager = villager - object;
	if (glm::length(toVillager) == 0.0f || glm::length(velocity) == 0.0f)
	{
		return false;
	}
	return glm::dot(glm::normalize(toVillager), glm::normalize(velocity)) >= k_ComingTowardsCosine;
}

bool openblack::magic::TimesOut(openblack::Reaction type)
{
	using openblack::Reaction;
	switch (type)
	{
	case Reaction::LookAtObject:
	case Reaction::ReactToNewBuilding:
	case Reaction::ReactToHandPickUp:
	case Reaction::ReactToHandUsingTotem:
	case Reaction::ReactToObjectCrushed:
	case Reaction::ReactToFight:
	case Reaction::LookAtNiceSpell:
	case Reaction::ReactToHandPuttingStuffInStoragePit:
	case Reaction::ReactToDeath:
	case Reaction::ReactToDroppedByHand:
	case Reaction::Fainting:
	case Reaction::Confused:
	case Reaction::AvoidFallingTree:
	case Reaction::CrowdAround:
	case Reaction::ReactToTownCelebration:
	case Reaction::ReactToVillagerInHand:
	case Reaction::ReactToMagicWaterPuttingOutFire:
	case Reaction::ReactToMagicShieldStruck:
	case Reaction::ReactToScaffold:
	case Reaction::ReactToFightWon:
		return true;
	default:
		return false;
	}
}

bool openblack::magic::TimedOut(openblack::Reaction type, std::optional<uint32_t> stamp, uint32_t age, uint32_t turns)
{
	return TimesOut(type) && stamp.has_value() && age - *stamp > turns;
}

float openblack::magic::StartingReach(bool grows, float maxDistance)
{
	return grows ? 1.0f : maxDistance;
}
