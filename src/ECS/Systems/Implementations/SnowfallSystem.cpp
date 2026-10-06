/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "SnowfallSystem.h"

#include <algorithm>

#include "3D/LandBlock.h"
#include "3D/LandIslandInterface.h"
#include "Common/GameRandom.h"
#include "ECS/Systems/WeatherSystemInterface.h"
#include "EngineConfig.h"
#include "Graphics/DetailLevel.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::systems;

namespace
{
/// A land block's side, which is in four quarters
constexpr float k_BlockSize = 160.0f;
/// The blocks whose snow is looked at, out to past where snow is drawn
constexpr float k_BlockReach = 400.0f + 160.0f;
/// Where the snow is looked at about a quarter's middle
constexpr float k_Around = 40.0f;
/// How high the flakes start when the snow starts
constexpr float k_StartHeight = 160.0f;

snowfall::Random GameRandom()
{
	return [](float a, float b) { return Locator::gameRandom::value().CrtRandom(a, b); };
}
} // namespace

SnowfallSystem::SnowfallSystem()
{
	Reset();
}

void SnowfallSystem::Reset()
{
	snowfall::Scatter(_flakes, GameRandom(), k_StartHeight);
	_drawn = false;
}

void SnowfallSystem::Update(float seconds, const rain::Fall& fall)
{
	if (_drawn)
	{
		_drawn = false;
		snowfall::Step(_flakes, seconds, fall.speed, fall.height, GameRandom());
	}
}

std::vector<snowfall::Tile> SnowfallSystem::TakeTiles(const glm::vec3& camera)
{
	std::vector<snowfall::Tile> tiles;
	// Snow is part of the weather setting
	if (!graphics::detail_level::Weather(Locator::config::value().detailLevel) || !Locator::terrainSystem::has_value() ||
	    !Locator::weatherSystem::has_value())
	{
		return tiles;
	}
	auto& weather = Locator::weatherSystem::value();
	const auto& island = Locator::terrainSystem::value();
	const glm::vec2 eye {camera.x, camera.z};
	for (const auto& block : island.GetBlocks())
	{
		const auto origin = block.GetMapPosition();
		if (!(glm::distance(origin + glm::vec2(k_BlockSize * 0.5f), eye) < k_BlockReach))
		{
			continue;
		}
		for (const auto& quarter :
		     std::array {glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f)})
		{
			// The most snow at four points about the quarter's middle
			const auto centre = origin + ((quarter + 0.5f) * snowfall::k_Span);
			int32_t snowiest = 0;
			for (const auto& offset :
			     std::array {glm::vec2(-1.0f, -1.0f), glm::vec2(-1.0f, 1.0f), glm::vec2(1.0f, -1.0f), glm::vec2(1.0f, 1.0f)})
			{
				const auto point = centre + (offset * k_Around);
				snowiest = std::max<int32_t>(snowiest, weather.GetWeather(glm::vec3(point.x, 0.0f, point.y)).snow);
			}
			if (auto tile = snowfall::TileOf(centre, snowiest, eye))
			{
				tile->ground = island.GetHeightAt(tile->corner);
				tiles.push_back(*tile);
			}
		}
	}
	_drawn = _drawn || !tiles.empty();
	return tiles;
}
