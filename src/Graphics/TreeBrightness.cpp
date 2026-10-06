/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TreeBrightness.h"

#include <cmath>

#include <glm/geometric.hpp>

using namespace openblack;

namespace
{
constexpr float k_Least = 200.0f;
constexpr float k_Range = 55.0f;

/// Normalised, or left as it is when it has no length
glm::vec3 Normalised(const glm::vec3& v)
{
	const auto lengthSquared = glm::dot(v, v);
	return lengthSquared > 0.0f ? v / std::sqrt(lengthSquared) : v;
}
} // namespace

int tree_brightness::Factor(const glm::vec3& cameraFocus, const glm::vec3& cameraForward, const glm::vec3& light)
{
	const auto awayFromLight = Normalised(cameraFocus - light);
	const auto facing = Normalised(glm::vec3(cameraForward.x, 0.0f, cameraForward.z));
	const float dot = glm::dot(facing, awayFromLight);
	if (dot < 0.0f)
	{
		return static_cast<int>(k_Least);
	}
	// Truncated
	return static_cast<int>(dot * k_Range + k_Least);
}
