/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "RainSystem.h"

#include <algorithm>
#include <array>
#include <optional>

#include <glm/geometric.hpp>

#include "3D/LandBlock.h"
#include "3D/LandIslandInterface.h"
#include "Common/GameRandom.h"
#include "ECS/Components/Weather.h"
#include "ECS/Registry.h"
#include "ECS/Systems/WeatherSystemInterface.h"
#include "EngineConfig.h"
#include "Graphics/DetailLevel.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::systems;

namespace
{
/// A land block's side, and the distance from its centre to the middle of its middle cells
constexpr float k_BlockSize = 160.0f;
constexpr float k_MiddleCells = 40.0f;
/// The blocks whose rain is looked at, out to past where rain is drawn
constexpr float k_BlockReach = 400.0f + 160.0f;

rain::Random GameRandom()
{
	return [](float a, float b) { return Locator::gameRandom::value().CrtRandom(a, b); };
}
} // namespace

RainSystem::RainSystem()
{
	Reset();
}

void RainSystem::Reset()
{
	const auto random = GameRandom();
	for (auto& streak : _streaks)
	{
		streak = rain::Place(random);
	}
	_fall = {};
	_drawn = false;
}

void RainSystem::Update(float seconds, const glm::vec3& camera)
{
	// The storm nearest the camera across the land sets how high and fast the rain falls
	std::optional<rain::Fall> nearest;
	float nearestDistance = 0.0f;
	Locator::entitiesRegistry::value().Each<const components::Storm>([&](const components::Storm& storm) {
		const glm::vec2 offset {storm.position.x - camera.x, storm.position.z - camera.z};
		const float distance = glm::dot(offset, offset);
		if (!nearest || distance < nearestDistance)
		{
			nearestDistance = distance;
			nearest = rain::Fall {.height = storm.cloudHeight, .speed = storm.rainSpeed};
		}
	});
	_fall = rain::Follow(_fall, nearest);
	if (_drawn)
	{
		_drawn = false;
		rain::Step(_streaks, seconds, _fall.speed, GameRandom());
	}
}

std::vector<rain::Tile> RainSystem::TakeTiles(const glm::vec3& camera)
{
	std::vector<rain::Tile> tiles;
	// Rain is part of the weather setting
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
		const auto centre = block.GetMapPosition() + glm::vec2(k_BlockSize * 0.5f);
		if (!(glm::distance(centre, eye) < k_BlockReach))
		{
			continue;
		}
		// The most rain over the block's four middle cells
		int32_t wettest = 0;
		for (const auto& offset :
		     std::array {glm::vec2(-1.0f, -1.0f), glm::vec2(-1.0f, 1.0f), glm::vec2(1.0f, -1.0f), glm::vec2(1.0f, 1.0f)})
		{
			const auto point = centre + (offset * k_MiddleCells);
			wettest = std::max<int32_t>(wettest, weather.GetWeather(glm::vec3(point.x, 0.0f, point.y)).rain);
		}
		if (auto tile = rain::TileOf(centre, wettest, eye))
		{
			tile->ground = island.GetHeightAt(centre);
			tiles.push_back(*tile);
		}
	}
	_drawn = _drawn || !tiles.empty();
	return tiles;
}
