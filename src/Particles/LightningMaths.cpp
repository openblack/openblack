/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "LightningMaths.h"

#include <cmath>

#include <algorithm>

#include <glm/geometric.hpp>

using namespace openblack::particles;

size_t maths::StrikeSearchCells(float radius)
{
	if (!(radius > 0.0f))
	{
		return 1;
	}
	const auto across = static_cast<size_t>(std::ceil(radius / k_StrikeCellSize));
	return 4 * across * across;
}

size_t maths::CloudSearchCells(float radius)
{
	if (!(radius > 0.0f))
	{
		return 1;
	}
	const auto across = static_cast<size_t>(std::ceil(radius / k_StrikeCellSize));
	return across * across;
}

size_t maths::ForkCount(int atOnce, size_t targets)
{
	return 2 * std::min(static_cast<size_t>(std::max(atOnce, 0)), targets) + 2;
}

std::vector<size_t> maths::PreferCreatures(const std::vector<bool>& isCreature)
{
	std::vector<size_t> creatures;
	for (size_t i = 0; i < isCreature.size(); ++i)
	{
		if (isCreature[i])
		{
			creatures.push_back(i);
		}
	}
	std::vector<size_t> result(isCreature.size());
	for (size_t i = 0; i < result.size(); ++i)
	{
		result[i] = creatures.empty() ? i : creatures[i % creatures.size()];
	}
	return result;
}

std::optional<glm::vec3> maths::ClashPoint(const BoltPose& newer, const BoltPose& older, float reach)
{
	const glm::vec3 heading(std::cos(newer.heading), 0.0f, std::sin(newer.heading));
	// They must point the same way across the land
	if (!(std::cos(older.heading) * heading.x + std::sin(older.heading) * heading.z > 0.0f))
	{
		return std::nullopt;
	}
	auto olderWay = older.centroid - older.origin;
	auto newerWay = older.centroid - newer.origin;
	const float olderLength = glm::length(olderWay);
	const float newerLength = glm::length(newerWay);
	if (olderLength > 0.0f)
	{
		olderWay /= olderLength;
	}
	const auto pull = (heading + olderWay) * 0.5f * k_ClashShare * std::min(olderLength, newerLength);
	const auto meeting = older.centroid - pull;
	const auto fromNewer = meeting - newer.origin;
	// Within the newer bolt's reach, and far enough in front of it (the game measures this without making the way unit
	// length)
	if (!(glm::length(fromNewer) < reach) ||
	    !(std::cos(k_ClashWidestAngle) < fromNewer.x * heading.x + fromNewer.z * heading.z))
	{
		return std::nullopt;
	}
	return meeting;
}

glm::vec3 maths::ArcTangent(glm::vec3 normal, glm::vec3 nudge, float randomTangents, float length, float scale)
{
	return (normal + nudge * randomTangents) * length * scale;
}

std::vector<glm::vec3> maths::ArcJoints(const ArcEnds& ends, glm::vec3 fromTangent, glm::vec3 toTangent,
                                        std::span<const glm::vec3> jitters, float randomFrac)
{
	const float length = glm::distance(ends.from, ends.to);
	// The cubic's coefficients from its ends and their tangents
	const auto a = 2.0f * ends.from - 2.0f * ends.to + fromTangent + toTangent;
	const auto b = -3.0f * ends.from + 3.0f * ends.to - 2.0f * fromTangent - toTangent;
	const auto c = fromTangent;
	const auto d = ends.from;
	std::vector<glm::vec3> joints;
	joints.reserve(jitters.size());
	const auto count = jitters.size();
	for (size_t k = 0; k < count; ++k)
	{
		const float t = count > 1 ? static_cast<float>(k) / static_cast<float>(count - 1) : 0.0f;
		joints.push_back(((a * t + b) * t + c) * t + d + jitters[k] * length * randomFrac);
	}
	return joints;
}
