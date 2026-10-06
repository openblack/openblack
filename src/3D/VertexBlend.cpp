/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "VertexBlend.h"

#include <cassert>
#include <cmath>

#include <algorithm>

#include <glm/common.hpp>

using namespace openblack;
using namespace openblack::vertex_blend;

namespace
{
constexpr float k_WeightSteps = 32767.0f;
} // namespace

std::vector<Partner> vertex_blend::Partners(std::span<const uint32_t> primitiveVertices,
                                            std::span<const uint32_t> primitiveBlends, std::span<const Blend> blends)
{
	assert(primitiveVertices.size() == primitiveBlends.size());
	uint32_t vertexCount = 0;
	for (const auto count : primitiveVertices)
	{
		vertexCount += count;
	}
	std::vector<Partner> partners(vertexCount);
	uint32_t firstVertex = 0;
	size_t nextBlend = 0;
	for (size_t p = 0; p < primitiveVertices.size(); ++p)
	{
		const auto vertices = primitiveVertices[p];
		for (uint32_t i = 0; i < primitiveBlends[p] && nextBlend < blends.size(); ++i, ++nextBlend)
		{
			const auto& blend = blends[nextBlend];
			if (blend.vertex >= vertices || blend.towards >= vertices || blend.vertex == blend.towards)
			{
				continue;
			}
			partners[firstVertex + blend.vertex] = {
			    .vertex = static_cast<int32_t>(firstVertex + blend.towards),
			    .weight = std::clamp(blend.weight, 0.0f, 1.0f),
			};
		}
		firstVertex += vertices;
	}
	return partners;
}

void vertex_blend::Apply(std::span<glm::vec3> positions, std::span<const Partner> partners)
{
	assert(positions.size() == partners.size());
	// Every vertex moves towards where its partner was placed, not where a blend of its own would take it
	const std::vector<glm::vec3> placed(positions.begin(), positions.end());
	for (size_t i = 0; i < partners.size(); ++i)
	{
		const auto& partner = partners[i];
		if (partner.Blended() && static_cast<size_t>(partner.vertex) < placed.size())
		{
			positions[i] = glm::mix(placed[i], placed[static_cast<size_t>(partner.vertex)], partner.weight);
		}
	}
}

int16_t vertex_blend::QuantiseWeight(float weight)
{
	return static_cast<int16_t>(std::lround(std::clamp(weight, 0.0f, 1.0f) * k_WeightSteps));
}

float vertex_blend::WeightOf(int16_t quantised)
{
	return static_cast<float>(quantised) / k_WeightSteps;
}
