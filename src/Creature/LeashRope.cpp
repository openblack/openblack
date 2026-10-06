/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "LeashRope.h"

#include <cmath>

#include <algorithm>
#include <tuple>

#include <glm/geometric.hpp>

using namespace openblack;
using namespace openblack::leash_rope;

namespace
{
/// The rope is divided into one more segment than it has masses
constexpr float k_Segments = static_cast<float>(k_NodeCount + 1);
/// Shorter than this, a direction is no direction
constexpr float k_Degenerate = 1e-6f;
/// How fast the texture repeats along the rope, per unit of length and the look's scale
constexpr float k_UScale = 0.05f;

/// The pull of a spring from p towards a neighbour: none while the segment is no longer than its rest, so that the
/// rope can go slack but not be squashed
glm::vec3 SpringPull(const glm::vec3& p, const glm::vec3& neighbour, float rest, float& stretch)
{
	const auto d = neighbour - p;
	const auto length = glm::length(d);
	if (length <= rest || length <= 0.0f)
	{
		stretch = 0.0f;
		return glm::vec3(0.0f);
	}
	stretch = (length / rest) - 1.0f;
	return k_Stiffness * (d - (d * (rest / length)));
}

/// The acceleration of each mass, from the springs to its neighbours, the air and gravity
std::array<glm::vec3, k_NodeCount> Accelerations(Rope& rope, float rest)
{
	std::array<glm::vec3, k_NodeCount> accelerations {};
	const auto dragLength = std::max(rest, k_MinDragLength);
	for (size_t i = 0; i < k_NodeCount; ++i)
	{
		auto& node = rope.nodes.at(i);
		const auto& previous = i == 0 ? rope.start : rope.nodes.at(i - 1).position;
		const auto& next = i + 1 == k_NodeCount ? rope.end : rope.nodes.at(i + 1).position;
		auto a = -k_Drag * glm::length(node.velocity) * node.velocity / dragLength;
		a.y += k_Gravity;
		float stretch = 0.0f;
		a += SpringPull(node.position, previous, rest, stretch);
		node.stretch = stretch;
		a += SpringPull(node.position, next, rest, stretch);
		accelerations.at(i) = a;
	}
	return accelerations;
}

void Integrate(Rope& rope, const std::array<glm::vec3, k_NodeCount>& accelerations, const GroundHeight& ground)
{
	constexpr float k_MaxSpeedSquared = k_MaxSpeed * k_MaxSpeed;
	for (size_t i = 0; i < k_NodeCount; ++i)
	{
		auto& node = rope.nodes.at(i);
		node.velocity += (k_StepSeconds / k_Mass) * accelerations.at(i);
		const auto speedSquared = glm::dot(node.velocity, node.velocity);
		if (speedSquared > k_MaxSpeedSquared)
		{
			node.velocity *= k_MaxSpeed / std::sqrt(speedSquared);
		}
		node.position += k_StepSeconds * node.velocity;
		const auto floor = ground(glm::vec2(node.position.x, node.position.z)) + rope.look.halfWidth + k_GroundClearance;
		node.position.y = std::max(node.position.y, floor);
	}
}
} // namespace

float leash_rope::RestLength(float slackLength)
{
	return slackLength / k_Segments;
}

Rope leash_rope::Create(const glm::vec3& start, const glm::vec3& end, float slackLength, float maxLength, const Look& look)
{
	Rope rope {
	    .start = start,
	    .end = end,
	    .nodes = {},
	    .slackLength = slackLength,
	    .maxLength = maxLength,
	    .look = look,
	    .tension = 0.0f,
	};
	for (size_t i = 0; i < k_NodeCount; ++i)
	{
		rope.nodes.at(i).position = start + ((end - start) * (static_cast<float>(i + 1) / k_Segments));
	}
	rope.tension = Tension(rope);
	return rope;
}

glm::vec3 leash_rope::ClampEnd(const glm::vec3& point)
{
	return {std::clamp(point.x, k_MinAcross, k_MaxAcross), std::clamp(point.y, k_MinHeight, k_MaxHeight),
	        std::clamp(point.z, k_MinAcross, k_MaxAcross)};
}

void leash_rope::Step(Rope& rope, const glm::vec3& start, const glm::vec3& end, float seconds, const GroundHeight& ground)
{
	const auto targetStart = ClampEnd(start);
	const auto targetEnd = ClampEnd(end);
	const auto steps = std::min(static_cast<uint32_t>(std::ceil(std::max(seconds, 0.0f) * k_StepsPerSecond)), k_MaxSteps);
	const auto rest = RestLength(rope.slackLength);
	const auto fromStart = rope.start;
	const auto fromEnd = rope.end;
	for (uint32_t step = 0; step < steps; ++step)
	{
		const auto t = static_cast<float>(step + 1) / static_cast<float>(steps);
		rope.start = fromStart + ((targetStart - fromStart) * t);
		rope.end = fromEnd + ((targetEnd - fromEnd) * t);
		Integrate(rope, Accelerations(rope, rest), ground);
	}
	rope.start = targetStart;
	rope.end = targetEnd;
	rope.tension = Tension(rope);
}

float leash_rope::Tension(const Rope& rope)
{
	const auto rest = RestLength(rope.slackLength);
	const auto taut = rope.maxLength / k_Segments;
	if (taut <= rest)
	{
		return 0.0f;
	}
	const auto first = glm::length(rope.nodes.front().position - rope.start);
	return std::clamp((first - rest) / (taut - rest), 0.0f, 1.0f);
}

glm::vec3 leash_rope::Point(const Rope& rope, size_t i)
{
	if (i == 0)
	{
		return rope.start;
	}
	if (i >= k_PointCount - 1)
	{
		return rope.end;
	}
	return rope.nodes.at(i - 1).position;
}

Ribbon leash_rope::BuildRibbon(const Rope& rope, const glm::vec3& eye, const GroundHeight& ground)
{
	Ribbon ribbon {};
	float travelled = 0.0f;
	for (size_t i = 0; i < k_PointCount; ++i)
	{
		const auto point = Point(rope, i);
		if (i > 0)
		{
			travelled += glm::distance(point, Point(rope, i - 1));
		}
		// The rope's direction at a point is from the point before it to the point after it
		auto tangent = Point(rope, std::min(i + 1, k_PointCount - 1)) - Point(rope, i == 0 ? 0 : i - 1);
		if (glm::length(tangent) < k_Degenerate)
		{
			tangent = glm::vec3(0.0f, 0.0f, 1.0f);
		}
		auto side = glm::cross(eye - point, tangent);
		side = glm::length(side) < k_Degenerate ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::normalize(side);
		side *= rope.look.halfWidth;
		const auto u = travelled * rope.look.uScale * k_UScale;
		ribbon.rope.at(i * 2) = {.position = point + side, .uv = {u, rope.look.v1}, .alpha = 1.0f};
		ribbon.rope.at((i * 2) + 1) = {.position = point - side, .uv = {u, rope.look.v0}, .alpha = 1.0f};

		// The shadow lies flat, across the rope's direction over the land
		auto flat = glm::vec3(-tangent.z, 0.0f, tangent.x);
		flat = glm::length(flat) < k_Degenerate ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::normalize(flat);
		flat *= rope.look.halfWidth;
		const bool atEnd = i == 0 || i == k_PointCount - 1;
		const auto alpha = atEnd ? 0.0f : static_cast<float>(k_ShadowAlpha) / 255.0f;
		for (const auto& [corner, offset, v] : {std::tuple {0, flat, rope.look.v1}, std::tuple {1, -flat, rope.look.v0}})
		{
			auto position = point + offset;
			position.y = ground(glm::vec2(position.x, position.z)) + k_ShadowLift;
			ribbon.shadow.at((i * 2) + static_cast<size_t>(corner)) = {.position = position, .uv = {u, v}, .alpha = alpha};
		}
	}
	return ribbon;
}

std::array<uint16_t, k_RibbonIndexCount> leash_rope::RibbonIndices()
{
	std::array<uint16_t, k_RibbonIndexCount> indices {};
	size_t index = 0;
	for (uint16_t i = 0; i + 1 < static_cast<uint16_t>(k_PointCount); ++i)
	{
		const auto a = static_cast<uint16_t>(i * 2);
		for (const auto offset : {0, 1, 2, 2, 1, 3})
		{
			indices.at(index++) = static_cast<uint16_t>(a + offset);
		}
	}
	return indices;
}
