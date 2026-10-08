/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>
#include <functional>

#include <glm/vec3.hpp>

/// A puff of smoke or dust: fifteen sprites of the smoke sheet scattered round a point, each flying off up and out,
/// spinning, growing and fading away over a second and a half. A blast throws up a brown one as it leaves its rubble, and a
/// tree taking root again a white one
namespace openblack::dust_puff
{

constexpr size_t k_Sprites = 15;
/// The dust's brown, 0xRRGGBB
constexpr uint32_t k_Colour = 0x68503Du;
/// Its life runs from 1 to 0 at this rate a second
constexpr float k_LifeRate = 2.0f / 3.0f;

struct Puff
{
	float life {1.0f};
	float size {1.0f};
	/// Its colour, 0xRRGGBB
	uint32_t colour {k_Colour};
	std::array<glm::vec3, k_Sprites> positions {};
	std::array<glm::vec3, k_Sprites> velocities {};
};

/// random(a, b) draws a number between them
[[nodiscard]] Puff Make(const glm::vec3& centre, float size, const std::function<float(float, float)>& random,
                        uint32_t colour = k_Colour);
/// A frame of seconds; false once it has gone
bool Advance(Puff& puff, float seconds);

/// How one sprite looks now
struct SpriteLook
{
	glm::vec3 position;
	float halfWidth;
	float angle;
	/// 0..255
	float alpha;
	/// Its cell of the smoke sheet
	int cell;
};
[[nodiscard]] std::array<SpriteLook, k_Sprites> Look(const Puff& puff);

} // namespace openblack::dust_puff
