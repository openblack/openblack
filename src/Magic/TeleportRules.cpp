/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TeleportRules.h"

#include <cmath>
#include <cstdlib>

#include <algorithm>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include "Common/GUtilsDistance.h"

using namespace openblack::magic;

namespace
{
/// A point across the land in the land's fixed point units
int32_t Fixed(float metres)
{
	return static_cast<int32_t>(metres * teleport::k_FixedUnitsPerMetre);
}

/// The distance across the land in metres
float Across(glm::vec3 a, glm::vec3 b)
{
	return glm::distance(glm::vec2(a.x, a.z), glm::vec2(b.x, b.z));
}

/// The distance across the land in metres as the game measures a jump: through its table of reciprocal roots, in whole
/// map units
float JumpDistance(glm::vec3 a, glm::vec3 b)
{
	return openblack::gutils::GetDistanceInMetres(openblack::map_coords::MapCoords {.x = Fixed(a.x), .z = Fixed(a.z)},
	                                              openblack::map_coords::MapCoords {.x = Fixed(b.x), .z = Fixed(b.z)});
}
} // namespace

int32_t teleport::FastDistance(glm::vec3 a, glm::vec3 b)
{
	const int32_t dx = std::abs(Fixed(b.x) - Fixed(a.x));
	const int32_t dz = std::abs(Fixed(b.z) - Fixed(a.z));
	return dx < dz ? (dx >> 1) + dz : (dz >> 1) + dx;
}

bool teleport::WithinAStep(glm::vec3 walker, glm::vec3 point, float stepMetres)
{
	const auto dx = static_cast<int64_t>(Fixed(point.x)) - Fixed(walker.x);
	const auto dz = static_cast<int64_t>(Fixed(point.z)) - Fixed(walker.z);
	const auto step = static_cast<int64_t>(Fixed(stepMetres));
	return (dx * dx) + (dz * dz) <= step * step;
}

int32_t teleport::CreatureFadeTurns(float turnMilliseconds)
{
	// The turns in a second, times the seconds, cut to a whole turn
	return turnMilliseconds > 0.0f ? static_cast<int32_t>((1000.0f / turnMilliseconds) * k_CreatureFadeSeconds) : 0;
}

float teleport::CreatureFadeOut(int32_t turnsLeft, int32_t turns)
{
	return turns > 0 ? 1.0f - (static_cast<float>(turnsLeft) / static_cast<float>(turns)) : 0.0f;
}

float teleport::CreatureFadeIn(int32_t turnsLeft, int32_t turns)
{
	return turns > 0 ? static_cast<float>(turnsLeft) / static_cast<float>(turns) : 0.0f;
}

bool teleport::IsWorthTheDetour(glm::vec3 traveller, glm::vec3 destination, glm::vec3 stone, glm::vec3 other)
{
	const auto walk = static_cast<float>(static_cast<uint32_t>(FastDistance(traveller, destination)));
	const auto detour =
	    static_cast<float>(static_cast<uint32_t>(FastDistance(traveller, stone) + FastDistance(other, destination)));
	return detour * k_DetourFactor < walk;
}

bool teleport::ShouldReact(glm::vec3 traveller, glm::vec3 destination, std::span<const glm::vec3> stones, size_t self)
{
	if (self >= stones.size())
	{
		return false;
	}
	for (size_t i = 0; i < stones.size(); ++i)
	{
		if (i != self && IsWorthTheDetour(traveller, destination, stones[self], stones[i]))
		{
			return true;
		}
	}
	return false;
}

std::optional<teleport::Jump> teleport::ChooseTarget(glm::vec3 traveller, glm::vec3 destination,
                                                     std::span<const glm::vec3> stones, size_t from, bool forced)
{
	float best = forced ? k_ForcedSaving : 0.0f;
	std::optional<Jump> chosen;
	const float walk = JumpDistance(destination, traveller);
	for (size_t i = 0; i < stones.size(); ++i)
	{
		if (i == from)
		{
			continue;
		}
		const float saving = walk - JumpDistance(destination, stones[i]);
		if (saving > best)
		{
			best = saving;
			chosen = Jump {.stone = i, .saving = saving};
		}
	}
	return chosen;
}

float teleport::JumpCost(float saving, float costPerKilometre)
{
	return -saving * costPerKilometre * k_MetresPerKilometre;
}

bool teleport::CanPlaceStone(glm::vec3 point, std::span<const glm::vec3> fixedObjects)
{
	return std::ranges::none_of(fixedObjects, [point](glm::vec3 object) { return Across(point, object) < k_StoneRadius; });
}

std::optional<size_t> teleport::FindRouteStone(glm::vec3 worshipper, glm::vec3 site, std::span<const glm::vec3> stones,
                                               float maxDistance)
{
	std::optional<size_t> nearest;
	float toWorshipper = maxDistance;
	float toSite = maxDistance;
	for (size_t i = 0; i < stones.size(); ++i)
	{
		if (const float d = Across(stones[i], worshipper); d < toWorshipper)
		{
			nearest = i;
			toWorshipper = d;
		}
		toSite = std::min(toSite, Across(stones[i], site));
	}
	if (toSite + toWorshipper < maxDistance)
	{
		return nearest;
	}
	return std::nullopt;
}

openblack::VillagerStates teleport::PreviousToKeep(VillagerStates final, VillagerStates keptBefore, bool finalKeepsPrevious)
{
	return finalKeepsPrevious ? keptBefore : final;
}

openblack::VillagerStates teleport::StateAfterReacting(VillagerStates comeBackTo)
{
	return comeBackTo == VillagerStates::InvalidState ? VillagerStates::DecideWhatToDo : comeBackTo;
}

bool teleport::CanDropOnStone(bool samePlayer, size_t playerStones)
{
	return samePlayer && playerStones != 1;
}
