/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "DustPuff.h"

#include <algorithm>

#include <glm/geometric.hpp>

using namespace openblack;
using namespace openblack::dust_puff;

namespace
{
/// Each sprite flies off at this many times the puff's size a second
constexpr float k_SpeedScale = 1.5f;
/// It is wholly opaque until its life falls below this share, then fades
constexpr float k_OpaqueLife = 0.7f;
/// Its spin by its life
constexpr float k_Spin = 5.0f;
/// Its cell runs back through so many of the sheet as its life runs out
constexpr float k_Cells = 15.0f;
constexpr float k_SmallestHalfWidth = 0.0001f;
} // namespace

Puff dust_puff::Make(const glm::vec3& centre, float size, const std::function<float(float, float)>& random, uint32_t colour)
{
	Puff puff;
	puff.size = size;
	puff.colour = colour;
	for (size_t i = 0; i < k_Sprites; ++i)
	{
		const float z = random(-size, size);
		const float y = random(-size, size);
		const float x = random(-size, size);
		puff.positions.at(i) = centre + glm::vec3(x, y, z);
		// Up and out
		glm::vec3 velocity(random(-size, size), size, random(-size, size));
		const float length = glm::length(velocity);
		puff.velocities.at(i) = length > 0.0f ? velocity * (size * k_SpeedScale / length) : glm::vec3(0.0f);
	}
	return puff;
}

bool dust_puff::Advance(Puff& puff, float seconds)
{
	puff.life -= seconds * k_LifeRate;
	if (puff.life <= 0.0f)
	{
		return false;
	}
	for (size_t i = 0; i < k_Sprites; ++i)
	{
		puff.positions.at(i) += puff.velocities.at(i) * seconds;
	}
	return true;
}

std::array<SpriteLook, k_Sprites> dust_puff::Look(const Puff& puff)
{
	std::array<SpriteLook, k_Sprites> looks {};
	const float alpha = puff.life < k_OpaqueLife ? puff.life * (1.0f / k_OpaqueLife) * 255.0f : 255.0f;
	const float halfWidth = std::max(((1.0f - puff.life) * 2.0f + 1.0f) * puff.size * 0.5f, k_SmallestHalfWidth);
	for (size_t i = 0; i < k_Sprites; ++i)
	{
		const auto& velocity = puff.velocities.at(i);
		const float turn = velocity.x <= velocity.z ? 1.0f : -1.0f;
		looks.at(i) = {
		    .position = puff.positions.at(i),
		    .halfWidth = halfWidth,
		    .angle = turn * puff.life * k_Spin + velocity.x,
		    .alpha = alpha,
		    .cell = static_cast<int>(puff.life * k_Cells) & 0x3F,
		};
	}
	return looks;
}
