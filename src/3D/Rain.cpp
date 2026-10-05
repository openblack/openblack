/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Rain.h"

#include <cmath>

#include <algorithm>

namespace openblack::rain
{

namespace
{
/// The half width of the square a streak stands in, about the block's centre
constexpr float k_Spread = 160.0f;
constexpr float k_Slant = 15.0f;
/// How far through its life a streak goes in a second
constexpr float k_LifeRate = 2.4f;
/// How far the rain's fall moves towards a storm's in a frame, and its bounds
constexpr float k_FollowRate = 0.3f;
constexpr float k_LowestHeight = 40.0f;
constexpr float k_HighestHeight = 640.0f;
constexpr float k_SlowestFall = 0.3f;
constexpr float k_FastestFall = 5.0f;
/// The least rain that shows, and the rain's opacity of 100 for each percent
constexpr int32_t k_LeastRain = 5;
constexpr int32_t k_Opacity = 88;
/// Rain is drawn up to this far from the camera, thinning out from the nearer distance
constexpr float k_Farthest = 400.0f;
constexpr float k_Thinning = 100.0f;
/// The streaks fade in and out over this much of their life
constexpr float k_Fade = 0.05f;

/// Drops what is left of a number past its whole part
float Fraction(float value)
{
	return value - static_cast<float>(static_cast<int32_t>(value));
}
} // namespace

Streak Place(const Random& random)
{
	Streak streak;
	streak.x = random(-k_Spread, k_Spread) * 0.5f;
	streak.z = random(-k_Spread, k_Spread) * 0.5f;
	streak.dx = random(-k_Slant, k_Slant);
	streak.dz = random(-k_Slant, k_Slant);
	streak.scroll = random(0.0f, 1.0f);
	streak.speed = random(0.1f, 0.2f);
	streak.phase = random(0.0f, 1.0f);
	return streak;
}

void Step(std::span<Streak> streaks, float seconds, float fallSpeed, const Random& random)
{
	const float life = seconds * k_LifeRate;
	for (auto& streak : streaks)
	{
		streak.scroll += fallSpeed * streak.speed * seconds;
		if (streak.scroll > 1.0f)
		{
			streak.scroll = Fraction(streak.scroll);
		}
		streak.phase += life;
		if (streak.phase > 1.0f)
		{
			// A new life somewhere else
			streak.phase = Fraction(streak.phase);
			streak.dx = random(-k_Slant, k_Slant);
			streak.dz = random(-k_Slant, k_Slant);
			streak.x = random(-k_Spread, k_Spread) * 0.5f;
			streak.z = random(-k_Spread, k_Spread) * 0.5f;
		}
	}
}

Fall Follow(Fall current, std::optional<Fall> storm)
{
	const auto target = storm.value_or(Fall {});
	current.height += (target.height - current.height) * k_FollowRate;
	current.speed += (target.speed - current.speed) * k_FollowRate;
	current.speed = !(current.speed > k_SlowestFall) ? k_SlowestFall : std::min(current.speed, k_FastestFall);
	current.height = !(current.height > k_LowestHeight) ? k_LowestHeight : std::min(current.height, k_HighestHeight);
	return current;
}

std::optional<Tile> TileOf(glm::vec2 centre, int32_t wettest, glm::vec2 camera)
{
	if (wettest <= k_LeastRain)
	{
		return std::nullopt;
	}
	Tile tile {.centre = centre, .streaks = static_cast<int32_t>(k_Streaks), .alpha = std::min(wettest * k_Opacity / 100, 255)};
	const auto offset = camera - centre;
	const float distance = std::sqrt((offset.x * offset.x) + (offset.y * offset.y));
	if (!(distance <= k_Farthest))
	{
		return std::nullopt;
	}
	if (distance > k_Thinning)
	{
		const double left = 1.0 - static_cast<double>((distance - k_Thinning) / (k_Farthest - k_Thinning));
		tile.alpha = static_cast<int32_t>(static_cast<double>(tile.alpha) * left);
		tile.streaks = static_cast<int32_t>(static_cast<double>(tile.streaks) * left);
	}
	if (tile.streaks <= 0)
	{
		return std::nullopt;
	}
	// The top is fainter, and fainter still further away
	const auto far = static_cast<float>((static_cast<double>(distance) / k_Farthest * 2.0) + 1.0);
	tile.alphaTop = static_cast<int32_t>(static_cast<double>(tile.alpha) / (static_cast<double>(far) * 5.0));
	return tile;
}

int32_t StreakAlpha(int32_t alpha, float phase)
{
	if (phase < k_Fade)
	{
		return static_cast<int32_t>(static_cast<double>(alpha) * phase * 20.0);
	}
	if (phase > 1.0f - k_Fade)
	{
		return static_cast<int32_t>(static_cast<double>(alpha) * ((1.0 - static_cast<double>(phase)) * 20.0));
	}
	return alpha;
}

std::array<glm::vec3, 2> Ends(const Streak& streak, float height)
{
	return {glm::vec3(streak.x, k_Bottom, streak.z), glm::vec3(streak.x + streak.dx, height, streak.z + streak.dz)};
}

} // namespace openblack::rain
