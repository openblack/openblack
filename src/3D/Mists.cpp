/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Mists.h"

#include <cmath>

#include <glm/geometric.hpp>

using namespace openblack;

float mists::EdgeOnSize(float size, float edgeShrink, const glm::vec3& toMist)
{
	const float lengthSquared = glm::dot(toMist, toMist);
	if (lengthSquared == 0.0f)
	{
		return size;
	}
	return size / (1.0f + ((edgeShrink - 1.0f) * (1.0f - (std::abs(toMist.y) / std::sqrt(lengthSquared)))));
}

void mists::Advance(int& counter, float& remainder, float milliseconds) noexcept
{
	remainder += milliseconds * k_Rate;
	const auto step = static_cast<int>(remainder);
	remainder -= static_cast<float>(step);
	counter += step;
	// Only once it passes the end
	if (counter > k_CounterWrap)
	{
		counter %= k_CounterWrap;
	}
}
