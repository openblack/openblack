/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureReach.h"

#include <cmath>

#include <algorithm>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

using namespace openblack;
using namespace openblack::creature_reach;

namespace
{
/// Spans narrower than this are taken as this, so nothing is divided by nothing
constexpr float k_TinySpan = 1e-4f;

float Ratio(float value, float from, float to)
{
	auto span = to - from;
	if (std::abs(span) < k_TinySpan)
	{
		span = span < 0.0f ? -k_TinySpan : k_TinySpan;
	}
	return (value - from) / span;
}

const glm::vec3& At(const Points& points, Corner corner)
{
	return points.at(static_cast<size_t>(corner));
}
} // namespace

Points creature_reach::Mirrored(const Points& points)
{
	auto mirrored = points;
	for (auto& point : mirrored)
	{
		point.x = -point.x;
	}
	return mirrored;
}

glm::vec3 creature_reach::Centre(const Points& points)
{
	glm::vec3 sum {0.0f};
	for (const auto& point : points)
	{
		sum += point;
	}
	return sum * 0.25f;
}

bool creature_reach::ReachesMirrored(const Points& points, const glm::vec3& target)
{
	const auto centre = Centre(points);
	const glm::vec2 own {centre.x, centre.z};
	const glm::vec2 reflected {-centre.x, centre.z};
	const glm::vec2 at {target.x, target.z};
	return glm::distance(at, reflected) < glm::distance(at, own);
}

Blend creature_reach::Solve(const Points& points, const glm::vec3& target)
{
	const auto& backLeft = At(points, Corner::BackLeft);
	const auto& backRight = At(points, Corner::BackRight);
	const auto& frontLeft = At(points, Corner::FrontLeft);
	const auto& frontRight = At(points, Corner::FrontRight);

	const auto s = Ratio(target.x, backLeft.x, backRight.x);
	const auto t = Ratio(target.x, frontLeft.x, frontRight.x);
	const auto backZ = backLeft.z + (s * (backRight.z - backLeft.z));
	const auto frontZ = frontLeft.z + (t * (frontRight.z - frontLeft.z));
	const auto d = Ratio(target.z, backZ, frontZ);

	const auto within = [](float value, float limit) { return value > -limit && value < 1.0f + limit; };
	return {
	    .acrossBack = s,
	    .acrossFront = t,
	    .backToFront = d,
	    .inRange = within(s, k_SideLimit) && within(t, k_SideLimit) && within(d, k_DepthLimit),
	    .weights = {(1.0f - s) * (1.0f - d), s * (1.0f - d), (1.0f - t) * d, t * d},
	};
}

float creature_reach::MaxReach(const Points& points)
{
	// The front pair stretched to the furthest depth the blend allows
	constexpr float k_Back = -k_DepthLimit / 2.0f;
	constexpr float k_Front = (1.0f + k_DepthLimit) / 2.0f;
	const auto furthest = (k_Back * (At(points, Corner::BackLeft) + At(points, Corner::BackRight))) +
	                      (k_Front * (At(points, Corner::FrontLeft) + At(points, Corner::FrontRight)));
	return glm::length(glm::vec2(furthest.x, furthest.z));
}
