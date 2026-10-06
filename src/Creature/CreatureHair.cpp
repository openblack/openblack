/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureHair.h"

#include <cassert>
#include <cmath>

#include <algorithm>

#include <glm/geometric.hpp>
#include <glm/matrix.hpp>

#include "3D/SkeletalAnimation.h"

using namespace openblack;
using namespace openblack::creature_hair;

namespace
{
/// A creature of size 1 is this tall
constexpr float k_HeightAtSizeOne = 15.0f;
/// A creature's head, and its hair, is this big for its size at size 0, shrinking by half the size up to size 2
constexpr float k_HeadScaleAtSizeZero = 1.8f;
constexpr float k_HeadScalePerSize = 0.5f;
constexpr float k_MaxHeadScaleSize = 2.0f;
/// The springs' stiffness is given in thousandths
constexpr float k_StiffnessScale = 1000.0f;
/// A bones' frame flatter than this has no inverse to turn a strand in
constexpr float k_MinFrameDeterminant = 1e-8f;

glm::vec3 Capped(const glm::vec3& value)
{
	const auto squared = glm::dot(value, value);
	if (squared > k_MaxSpeed * k_MaxSpeed)
	{
		return value * (k_MaxSpeed / std::sqrt(squared));
	}
	return value;
}

glm::vec3 NormalisedOr(const glm::vec3& value, const glm::vec3& fallback)
{
	const auto length = glm::length(value);
	return length > 0.0f ? value / length : fallback;
}
} // namespace

float creature_hair::ByAlignment(const std::array<float, 3>& values, float alignment)
{
	if (alignment < 0.0f)
	{
		return (values[0] * (1.0f + alignment)) - (values[1] * alignment);
	}
	return (values[0] * (1.0f - alignment)) + (values[2] * alignment);
}

int32_t creature_hair::ByAlignment(const std::array<int32_t, 3>& values, float alignment)
{
	const std::array<float, 3> asFloats {static_cast<float>(values[0]), static_cast<float>(values[1]),
	                                     static_cast<float>(values[2])};
	return static_cast<int32_t>(ByAlignment(asFloats, alignment));
}

Look creature_hair::LookFor(const Variants& variants, float alignment)
{
	// A float member of the look, written as an alias that formats the same with every clang-format
	using FloatMember = float Look::*;
	const auto floats = [&variants, alignment](FloatMember member) {
		return ByAlignment(std::array {variants[0].*member, variants[1].*member, variants[2].*member}, alignment);
	};
	const auto channel = [&variants, alignment](glm::length_t c) {
		return ByAlignment(std::array {variants[0].colour[c], variants[1].colour[c], variants[2].colour[c]}, alignment);
	};
	return {
	    .colour = {channel(0), channel(1), channel(2)},
	    .length = floats(&Look::length),
	    .damping = floats(&Look::damping),
	    .stiffness = floats(&Look::stiffness),
	    .thickness = floats(&Look::thickness),
	};
}

float creature_hair::HairScale(float size)
{
	const auto headScale = k_HeadScaleAtSizeZero - (k_HeadScalePerSize * std::clamp(size, 0.0f, k_MaxHeadScaleSize));
	return headScale * size * k_HeightAtSizeOne;
}

Physics creature_hair::PhysicsFor(const Look& look, float scale, uint32_t segmentCount)
{
	const auto keptOverASecond = look.damping * look.length;
	const auto depth = look.thickness * scale * 0.5f;
	return {
	    .segmentLength = segmentCount > 0 ? look.length * scale / static_cast<float>(segmentCount) : 0.0f,
	    .stiffness = scale > 0.0f && look.length > 0.0f ? look.stiffness * k_StiffnessScale / scale / look.length : 0.0f,
	    .damping = keptOverASecond * keptOverASecond,
	    .rootDepth = depth,
	    .halfWidth = depth,
	};
}

glm::vec3 creature_hair::GrowthDirection(const glm::vec3& inwardNormal)
{
	return NormalisedOr(-inwardNormal, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 creature_hair::GrowthDirection(const glm::vec3& inwardNormal, const glm::mat3& frame, const glm::vec3& angles)
{
	// The game's rotation is used with row vectors; as a matrix of columns it is its transpose, whose columns are the
	// game's rows
	const auto rows = skeletal_animation::RotationYXZ(angles);
	glm::mat3 turn;
	for (glm::length_t c = 0; c < 3; ++c)
	{
		for (glm::length_t r = 0; r < 3; ++r)
		{
			turn[c][r] = rows.at(static_cast<size_t>(c)).at(static_cast<size_t>(r));
		}
	}
	// A frame that has collapsed flat can't be undone, so the strand grows straight out
	if (std::abs(glm::determinant(frame)) <= k_MinFrameDeterminant)
	{
		return GrowthDirection(inwardNormal);
	}
	return NormalisedOr(frame * turn * glm::inverse(frame) * -inwardNormal, GrowthDirection(inwardNormal));
}

glm::mat3 creature_hair::BoneFrame(std::span<const glm::mat3> bones)
{
	const auto unitAxes = [](const glm::mat3& matrix) {
		return glm::mat3(NormalisedOr(matrix[0], glm::vec3(1.0f, 0.0f, 0.0f)),
		                 NormalisedOr(matrix[1], glm::vec3(0.0f, 1.0f, 0.0f)),
		                 NormalisedOr(matrix[2], glm::vec3(0.0f, 0.0f, 1.0f)));
	};
	glm::mat3 sum(0.0f);
	for (const auto& bone : bones)
	{
		sum += unitAxes(bone);
	}
	return unitAxes(sum);
}

glm::ivec3 creature_hair::StrandColour(const glm::ivec3& colour, const glm::ivec3& light, const glm::ivec3& added)
{
	return glm::min((glm::clamp(colour, 0, 255) * glm::clamp(light, 0, 255)) / 255 + glm::max(added, 0), glm::ivec3(255));
}

Strand creature_hair::Straight(const Root& root, const Physics& physics, uint32_t segmentCount)
{
	Strand strand;
	strand.positions.resize(segmentCount);
	strand.velocities.assign(segmentCount, glm::vec3(0.0f));
	const auto start = root.position + (root.inwardNormal * physics.rootDepth);
	for (uint32_t i = 0; i < segmentCount; ++i)
	{
		strand.positions[i] = start + (root.direction * (physics.segmentLength * static_cast<float>(i)));
	}
	return strand;
}

void creature_hair::Step(Strand& strand, const Root& root, const Physics& physics, float seconds)
{
	auto& positions = strand.positions;
	auto& velocities = strand.velocities;
	assert(positions.size() == velocities.size());
	if (positions.empty())
	{
		return;
	}
	// The root is sunk a little into the body
	positions[0] = root.position + (root.inwardNormal * physics.rootDepth);
	const auto before = positions;
	const auto kept = std::pow(physics.damping, seconds);

	// Each point is pulled towards lying straight on from the segment before it, as the strand lay at the start of
	// the step; the first towards growing out of the surface
	auto direction = root.direction;
	for (size_t i = 1; i < positions.size(); ++i)
	{
		const auto target = positions[i - 1] + (direction * physics.segmentLength);
		const auto pull = Capped((target - positions[i]) * physics.stiffness);
		velocities[i] += glm::vec3(pull.x, pull.y - k_Gravity, pull.z) * seconds;
		velocities[i] = Capped(velocities[i]) * kept;
		direction = NormalisedOr(positions[i] - positions[i - 1], direction);
	}
	for (size_t i = 1; i < positions.size(); ++i)
	{
		positions[i] += velocities[i] * seconds;
	}
	// The strand doesn't stretch: each point is put back a segment from the one before it
	for (size_t i = 1; i < positions.size(); ++i)
	{
		const auto offset = NormalisedOr(positions[i] - positions[i - 1], direction) * physics.segmentLength;
		positions[i] = positions[i - 1] + offset;
	}
	if (seconds > k_MinStepSeconds)
	{
		for (size_t i = 1; i < positions.size(); ++i)
		{
			velocities[i] = (positions[i] - before[i]) / seconds;
		}
	}
}

void creature_hair::BuildRibbon(std::span<const glm::vec3> points, const glm::vec3& eye, float halfWidth,
                                std::span<RibbonVertex> out)
{
	assert(out.size() == points.size() * 2);
	if (points.size() < 2)
	{
		return;
	}
	const auto last = static_cast<float>(points.size() - 1);
	for (size_t i = 0; i < points.size(); ++i)
	{
		const auto from = i == 0 ? size_t {0} : i - 1;
		const auto across = glm::cross(points[from + 1] - points[from], points[from] - eye);
		const auto length = glm::length(across);
		const auto side = length > 0.0f ? across * (halfWidth / length) : glm::vec3(0.0f);
		const auto u = static_cast<float>(i) / last;
		out[i * 2] = {.position = points[i] + side, .uv = {u, 0.0f}};
		out[(i * 2) + 1] = {.position = points[i] - side, .uv = {u, k_RibbonTextureHeight}};
	}
}
