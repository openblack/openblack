/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "WaterRings.h"

#include <cmath>

#include <algorithm>

namespace openblack::water_rings
{

namespace
{
/// How the ring grows and fades over its life: its width by 1/700 of its age, its opacity down by 255/700 of it
constexpr float k_GrowthPerMillisecond = 1.0f / 700.0f;
constexpr float k_FadePerMillisecond = 255.0f / 700.0f;
constexpr float k_SmallestHalfWidth = 0.0001f;
/// The hand's splash
constexpr float k_SplashHeight = 0.2f;
constexpr float k_SplashGrowth = 7.0f;
constexpr uint8_t k_SplashCell = 0x30;
constexpr uint32_t k_SplashAlpha = 0xB0u;
/// The texture's cells across and down
constexpr float k_Cell = 1.0f / 8.0f;
} // namespace

Ring HandSplash(glm::vec2 xz, float angle, uint32_t landLight)
{
	return {
	    .position = {xz.x, k_SplashHeight, xz.y},
	    .growth = k_SplashGrowth,
	    .angle = angle,
	    .cell = k_SplashCell,
	    .argb = (k_SplashAlpha << 24u) | (landLight & 0xFFFFFFu),
	};
}

bool Advance(Ring& ring, float milliseconds)
{
	ring.age += static_cast<uint32_t>(milliseconds * ring.rate);
	return ring.age < k_Life;
}

float HalfWidth(const Ring& ring)
{
	return std::max(static_cast<float>(ring.age) * ring.growth * k_GrowthPerMillisecond, k_SmallestHalfWidth);
}

uint8_t Alpha(const Ring& ring)
{
	const float fading = 255.0f - (static_cast<float>(ring.age % k_Life) * k_FadePerMillisecond);
	const auto alpha = static_cast<int32_t>(fading * static_cast<float>(ring.argb >> 24u));
	return static_cast<uint8_t>((alpha >> 8) & 0xFF);
}

std::array<glm::vec3, 4> Corners(const Ring& ring)
{
	const float half = HalfWidth(ring);
	const glm::vec2 across = glm::vec2(std::cos(ring.angle), -std::sin(ring.angle)) * half;
	const glm::vec2 along = glm::vec2(std::sin(ring.angle), std::cos(ring.angle)) * (half * ring.aspect);
	const auto at = [&ring](glm::vec2 offset) {
		return glm::vec3(ring.position.x + offset.x, ring.position.y, ring.position.z + offset.y);
	};
	return {at(-across - along), at(across - along), at(across + along), at(-across + along)};
}

std::array<glm::vec2, 4> CellUvs(uint8_t cell)
{
	const auto index = static_cast<uint8_t>(cell & 0x3Fu);
	const glm::vec2 corner {static_cast<float>(index % 8u) * k_Cell, static_cast<float>(index / 8u) * k_Cell};
	return {corner, corner + glm::vec2(k_Cell, 0.0f), corner + glm::vec2(k_Cell), corner + glm::vec2(0.0f, k_Cell)};
}

} // namespace openblack::water_rings
