/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureCastMoves.h"

#include <cmath>

#include <algorithm>
#include <limits>
#include <vector>

#include <glm/geometric.hpp>

#include "Creature/CreatureLocomotion.h"

using namespace openblack;
using namespace openblack::creature_cast_moves;

namespace
{
/// Walking up to something much lower than this share of its own height, the creature keeps clear of it by most of its
/// own radius less; up to that height less and less, and past it by this much
constexpr float k_LowThingShare = 0.8f;
constexpr float k_LowThingKeep = 0.7f;
constexpr float k_TallerThingKeep = 0.3f;
constexpr float k_TallThingKeep = 0.4f;
/// A tree is walked up to within this share of its radius, at most this close
constexpr float k_TreeShare = 0.1f;
constexpr float k_TreeMost = 0.25f;
/// A thing longer than this many times its width keeps a row of circles clear
constexpr float k_LongThing = 1.4f;
} // namespace

float creature_cast_moves::BoneReach(const skeletal_animation::Animation& animation,
                                     const skeletal_animation::Skeleton& skeleton)
{
	if (skeleton.Empty())
	{
		return 0.0f;
	}
	const auto poses = skeletal_animation::SampleCycle(animation, animation, 0, skeleton);
	const auto bones = skeletal_animation::ComposeBoneMatrices(poses, skeleton.parents);
	float reach = 0.0f;
	for (const auto& bone : bones)
	{
		reach = std::max(reach, std::sqrt(bone[3].x * bone[3].x + bone[3].z * bone[3].z));
	}
	return reach;
}

float creature_cast_moves::CreatureRadius(float modelScale, float boneReach)
{
	return modelScale * boneReach;
}

float creature_cast_moves::RoutePlanRadius(float radius, float height, bool tree, float creatureHeight, float creatureRadius)
{
	if (tree)
	{
		return std::min(k_TreeShare * radius, k_TreeMost);
	}
	const float low = creatureHeight * k_LowThingShare;
	const float keep = height <= low ? k_LowThingKeep - k_TallerThingKeep * height / low : k_TallThingKeep;
	return radius - keep * creatureRadius;
}

bool creature_cast_moves::Arrived(float distance, float creatureRadius, float routeRadius, float keep)
{
	return distance < k_ArrivalMargin * (creatureRadius + routeRadius + keep);
}

float creature_cast_moves::GetAwayDistance(float keep, std::optional<float> thingRadius)
{
	return keep + (thingRadius.has_value() ? *thingRadius * k_GetAwayRadii : 0.0f);
}

glm::vec3 creature_cast_moves::GetAwayPoint(const glm::vec3& creature, const glm::vec3& thing, float distance)
{
	const auto away = creature - thing;
	const float flat = glm::length(glm::vec2(away.x, away.z));
	glm::vec3 direction {0.0f};
	if (!(flat > 0.0001f))
	{
		direction = {1.0f, 0.0f, 0.0f};
	}
	else if (const float length = glm::length(away); length > 0.0f)
	{
		direction = away / length;
	}
	return creature + direction * distance;
}

std::optional<glm::vec2> creature_cast_moves::FindClearArea(glm::vec2 point, float width, const ClearCell& clear)
{
	const auto centreX = static_cast<int32_t>(std::floor(point.x / k_CellSize));
	const auto centreZ = static_cast<int32_t>(std::floor(point.y / k_CellSize));
	const int32_t originX = centreX - k_ClearAreaCells / 2;
	const int32_t originZ = centreZ - k_ClearAreaCells / 2;
	std::vector<bool> open(static_cast<size_t>(k_ClearAreaCells * k_ClearAreaCells));
	for (int32_t z = 0; z < k_ClearAreaCells; ++z)
	{
		for (int32_t x = 0; x < k_ClearAreaCells; ++x)
		{
			open.at(static_cast<size_t>(x + z * k_ClearAreaCells)) = clear(originX + x, originZ + z);
		}
	}
	// A square of cells as wide as the area, rounded up to whole cells
	const auto whole = static_cast<int32_t>(width);
	const int32_t cells = static_cast<int32_t>(width / k_CellSize) + (whole % static_cast<int32_t>(k_CellSize) != 0 ? 1 : 0);
	const float half = width * 0.5f;
	const auto inDisc = [&](int32_t i, int32_t j) {
		const float dx = half - (static_cast<float>(i) * k_CellSize + k_CellSize * 0.5f);
		const float dz = half - (static_cast<float>(j) * k_CellSize + k_CellSize * 0.5f);
		return dx * dx + dz * dz <= 0.25f * width * width;
	};
	std::optional<glm::vec2> best;
	float nearest = std::numeric_limits<float>::max();
	for (int32_t a = 0; a < k_ClearAreaCells - cells; ++a)
	{
		for (int32_t b = 0; b < k_ClearAreaCells - cells; ++b)
		{
			bool fits = true;
			for (int32_t i = 0; i < cells && fits; ++i)
			{
				for (int32_t j = 0; j < cells && fits; ++j)
				{
					fits = !inDisc(i, j) || open.at(static_cast<size_t>(a + i + (b + j) * k_ClearAreaCells));
				}
			}
			if (!fits)
			{
				continue;
			}
			const glm::vec2 candidate {static_cast<float>(originX + a) * k_CellSize + half,
			                           static_cast<float>(originZ + b) * k_CellSize + half};
			const float distance = glm::distance(candidate, point);
			if (distance < nearest)
			{
				nearest = distance;
				best = candidate;
			}
		}
	}
	return best;
}

bool creature_cast_moves::Facing(glm::vec2 creature, float heading, glm::vec2 point)
{
	const auto offset = point - creature;
	if (glm::length(offset) < k_OnTopDistance)
	{
		return true;
	}
	const float off = creature_locomotion::WrapAngle(creature_locomotion::HeadingOf(offset) - heading);
	return !(std::abs(off) > k_FacingRadians);
}

std::vector<CollideCircle> creature_cast_moves::CollideCircles(glm::vec2 halfSize, glm::vec2 centre, const glm::mat2& turn)
{
	const auto half = glm::max(halfSize, glm::vec2(1.0f));
	const float longer = std::max(half.x, half.y);
	const float shorter = std::min(half.x, half.y);
	if (!(longer / shorter > k_LongThing))
	{
		return {{.centre = centre, .radius = longer}};
	}
	// A row of circles as wide as its shorter side, spread evenly along its longer
	const auto count = static_cast<int32_t>(longer / shorter) + 1;
	const float spacing = 2.0f * longer / static_cast<float>(count);
	std::vector<CollideCircle> circles;
	circles.reserve(static_cast<size_t>(count));
	for (int32_t i = 0; i < count; ++i)
	{
		const float along = (static_cast<float>(i) + 0.5f) * spacing - longer;
		const glm::vec2 local = half.x > half.y ? glm::vec2(along, 0.0f) : glm::vec2(0.0f, along);
		circles.push_back({.centre = centre + turn * local, .radius = shorter});
	}
	return circles;
}

bool creature_cast_moves::BlocksCell(const CollideCircle& circle, int32_t x, int32_t z)
{
	const glm::vec2 middle {static_cast<float>(x) * k_CellSize + k_CellSize * 0.5f,
	                        static_cast<float>(z) * k_CellSize + k_CellSize * 0.5f};
	return glm::distance(middle, circle.centre) - circle.radius < k_ClearOfThings;
}
