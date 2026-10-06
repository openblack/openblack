/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureShadow.h"

#include <cmath>

#include <algorithm>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>

namespace openblack::graphics
{

uint8_t CreatureShadow::Alpha(float cameraDistance, float radius)
{
	if (radius <= 0.0f)
	{
		return 0;
	}
	const auto radii = cameraDistance / radius;
	if (radii < k_FadeStart)
	{
		return 255;
	}
	if (radii < k_FadeEnd)
	{
		return static_cast<uint8_t>(255.0f * (k_FadeEnd - radii) / (k_FadeEnd - k_FadeStart));
	}
	return 0;
}

glm::vec3 CreatureShadow::LightPoint(const glm::vec3& light, const glm::vec3& centre, float radius)
{
	auto towards = light - centre;
	const auto nearest = k_LightDistance * radius;
	auto across = std::sqrt((towards.x * towards.x) + (towards.z * towards.z));
	if (across < nearest)
	{
		if (across < k_LightNudge)
		{
			towards.x += 1.0f;
			towards.z += 1.0f;
		}
		const auto length = glm::length(towards);
		towards *= nearest / length;
		across = std::sqrt((towards.x * towards.x) + (towards.z * towards.z));
	}
	// No lower than 45 degrees
	if (towards.y < across)
	{
		towards.y = across;
	}
	return centre + towards;
}

glm::mat4 CreatureShadow::CellMatrix(uint8_t cell, uint8_t cells)
{
	const auto count = static_cast<float>(std::max<uint8_t>(cells, 1));
	const auto inner = static_cast<float>(k_CellSize - (2 * k_CellBorder)) / static_cast<float>(k_CellSize);
	auto matrix = glm::mat4(1.0f);
	matrix[0][0] = inner / count;
	matrix[1][1] = inner;
	matrix[3][0] = -1.0f + ((2.0f * static_cast<float>(cell) + 1.0f) / count);
	return matrix;
}

std::optional<CreatureShadow> CreatureShadow::Compute(const glm::vec3& centre, float radius, float groundHeight,
                                                      const glm::vec3& light, const glm::vec3& camera, uint8_t cell,
                                                      uint8_t cells, bool originBottomLeft, bool homogeneousDepth)
{
	const auto ground = glm::vec3(centre.x, groundHeight, centre.z);
	const auto alpha = Alpha(glm::distance(camera, ground), radius);
	if (alpha == 0)
	{
		return std::nullopt;
	}
	const auto from = LightPoint(light, centre, radius);
	const auto direction = glm::normalize(centre - from);

	CreatureShadow shadow {};
	shadow.strength = static_cast<float>(alpha) / 255.0f;
	shadow.startDepth = -0.5f * radius;
	const auto up = std::abs(direction.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
	shadow.view = glm::lookAtRH(centre - (direction * (radius * 2.0f)), centre, up);
	shadow.projection = homogeneousDepth ? glm::orthoRH_NO(-radius, radius, -radius, radius, 0.0f, radius * 4.0f)
	                                     : glm::orthoRH_ZO(-radius, radius, -radius, radius, 0.0f, radius * 4.0f);

	// From the cell's clip space to texture coordinates, which run downwards on some renderers
	auto bias = glm::mat4(1.0f);
	bias[0][0] = 0.5f;
	bias[1][1] = originBottomLeft ? 0.5f : -0.5f;
	bias[3][0] = 0.5f;
	bias[3][1] = 0.5f;
	shadow.receiverMatrix = bias * CellMatrix(cell, cells) * shadow.projection * shadow.view;
	// Distance past the creature's centre along the light
	for (glm::length_t column = 0; column < 3; ++column)
	{
		shadow.receiverMatrix[column][2] = direction[column];
	}
	shadow.receiverMatrix[3][2] = -glm::dot(centre, direction);
	return shadow;
}

} // namespace openblack::graphics
