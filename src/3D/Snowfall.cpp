/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Snowfall.h"

#include <cmath>

#include <algorithm>
#include <numbers>

#include <glm/gtx/euler_angles.hpp>
#include <glm/mat4x4.hpp>

namespace openblack::snowfall
{

namespace
{
constexpr float k_Pi = std::numbers::pi_v<float>;
constexpr float k_TwoPi = 2.0f * k_Pi;
/// How far a flake drifts round its circle, units a second
constexpr float k_Drift = 3.0f;
/// The highest the flakes start when the snow starts
constexpr float k_ScatterTop = 160.0f;
/// Less snow than this isn't drawn
constexpr int32_t k_LeastSnow = 5;
/// The quarters are drawn out to here from the camera, fainter from the nearer distance
constexpr float k_Farthest = 300.0f;
constexpr float k_Fading = 50.0f;
} // namespace

Flake Place(const Random& random, float height)
{
	Flake flake;
	flake.position.z = random(-80.0f, 80.0f) * 0.5f;
	flake.position.y = random(0.0f, height);
	flake.position.x = random(-80.0f, 80.0f) * 0.5f;
	flake.fall = random(0.0f, 1.0f) + 7.0f;
	flake.swaySpeed = random(0.0f, 10.0f) - 5.0f;
	flake.heading = random(0.0f, k_TwoPi);
	flake.sway = random(0.0f, k_Pi);
	flake.spin.z = random(0.0f, k_TwoPi);
	flake.spin.y = random(0.0f, k_TwoPi);
	flake.spin.x = random(0.0f, k_TwoPi);
	flake.spinSpeed.z = random(0.0f, k_Pi * 0.5f);
	flake.spinSpeed.y = random(0.0f, k_Pi * 0.5f);
	flake.spinSpeed.x = random(0.0f, k_Pi * 0.5f);
	return flake;
}

void Scatter(std::span<Flake> flakes, const Random& random, float height)
{
	for (auto& flake : flakes)
	{
		flake = Place(random, height);
		flake.position.y = random(k_Bottom, k_ScatterTop);
	}
}

void Step(std::span<Flake> flakes, float seconds, float fallSpeed, float height, const Random& random)
{
	for (auto& flake : flakes)
	{
		if (flake.position.y <= k_Bottom)
		{
			flake = Place(random, height);
		}
		flake.position.y -= fallSpeed * flake.fall * seconds;
		// Each time round its circle it turns a little either way
		flake.sway += fallSpeed * flake.swaySpeed * seconds;
		if (flake.sway > k_TwoPi)
		{
			flake.sway -= k_TwoPi;
			flake.heading = (flake.heading - (k_Pi / 8.0f)) + random(0.0f, k_Pi / 4.0f);
			if (flake.heading > k_TwoPi)
			{
				flake.heading -= k_TwoPi;
			}
			if (flake.heading < 0.0f)
			{
				flake.heading += k_TwoPi;
			}
		}
		const float drift = std::sin(flake.sway) * seconds * k_Drift;
		flake.position.x += std::sin(flake.heading) * drift;
		flake.position.z += std::cos(flake.heading) * drift;
		flake.position.y -= std::cos(flake.sway) * seconds * k_Drift * 0.5f;
		flake.spin += seconds * flake.spinSpeed;
	}
}

std::array<glm::vec3, 4> Corners(const Flake& flake)
{
	const auto turn = glm::eulerAngleYXZ(flake.spin.x, flake.spin.y, flake.spin.z);
	const auto across = glm::vec3(turn[0][0], turn[1][0], turn[2][0]) * k_HalfSize;
	const auto up = glm::vec3(turn[0][2], turn[1][2], turn[2][2]) * k_HalfSize;
	const auto& middle = flake.position;
	return {middle + across + up, middle + across - up, middle - across - up, middle - across + up};
}

std::optional<Tile> TileOf(glm::vec2 centre, int32_t snowiest, glm::vec2 camera)
{
	if (snowiest <= k_LeastSnow)
	{
		return std::nullopt;
	}
	// Twice as many flakes as the snow's opacity, all of them from half of it
	const int32_t alpha = std::min((snowiest * 256) / 100, 255);
	const int32_t flakes = std::min(alpha * 2, static_cast<int32_t>(k_Flakes));
	if (flakes <= k_LeastSnow)
	{
		return std::nullopt;
	}
	// The flakes fall about the quarter's corner, in whole quarters across the land
	const glm::vec2 corner {std::trunc(centre.x / k_Span) * k_Span, std::trunc(centre.y / k_Span) * k_Span};
	const auto offset = camera - corner;
	const float distance = std::sqrt((offset.x * offset.x) + (offset.y * offset.y));
	if (distance > k_Farthest)
	{
		return std::nullopt;
	}
	Tile tile {.corner = corner, .flakes = flakes, .alpha = alpha};
	if (distance > k_Fading)
	{
		tile.alpha =
		    static_cast<int32_t>(static_cast<float>(alpha) * (1.0f - ((distance - k_Fading) / (k_Farthest - k_Fading))));
	}
	if (tile.alpha <= 4)
	{
		return std::nullopt;
	}
	return tile;
}

} // namespace openblack::snowfall
