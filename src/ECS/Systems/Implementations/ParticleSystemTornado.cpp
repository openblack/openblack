/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// What a tornado's particles ask of the world: the game turn, the colour of the land at its foot, and what it reaches,
// picks up and catches, which the tornado system does

#define LOCATOR_IMPLEMENTATIONS

#include <LNDFile.h>
#include <glm/common.hpp>

#include "3D/LandIslandInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "ECS/Systems/TornadoSystemInterface.h"
#include "ECS/Systems/WeatherSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "ParticleSystem.h"

using namespace openblack;
using namespace openblack::ecs::systems;

namespace
{
constexpr float k_CellSize = 10.0f;
/// Land of a lower altitude than this is under the sea
constexpr uint8_t k_LowestDryAltitude = 4;
/// Where the snow lying at a cell's corner is this deep or deeper, the land throws up snow
constexpr int8_t k_SnowDust = 27;
/// The material snow is, a cell's material of none is the first, and past the last there are none
constexpr uint32_t k_SnowMaterial = 27;
constexpr uint32_t k_NoMaterial = 0;
constexpr uint32_t k_FirstMaterial = 1;
constexpr uint32_t k_LastMaterial = 43;
} // namespace

uint32_t GameParticleWorld::GameTurn() const
{
	return Locator::time::has_value() ? Locator::time::value().GetTurn() : 0;
}

glm::u8vec3 GameParticleWorld::LandColour(glm::vec2 xz) const
{
	constexpr glm::u8vec3 k_White {255, 255, 255};
	if (!Locator::terrainSystem::has_value() || !Locator::infoConstants::has_value())
	{
		return k_White;
	}
	// The tornado's dust colour of the land's material at the cell: snow where it lies deep at the cell's corner, else
	// the material the land's country lays at the cell's altitude; a cell of none, or off the land, is of the first
	const auto& land = Locator::terrainSystem::value();
	const auto cellIndex = glm::floor(xz / k_CellSize);
	uint32_t material = k_NoMaterial;
	const glm::vec3 corner(cellIndex.x * k_CellSize, 0.0f, cellIndex.y * k_CellSize);
	if (Locator::weatherSystem::has_value() && Locator::weatherSystem::value().GetWeather(corner).snowCover >= k_SnowDust)
	{
		material = k_SnowMaterial;
	}
	else
	{
		const auto* cell = xz.x < 0.0f || xz.y < 0.0f ? nullptr : land.FindCell(glm::u16vec2(cellIndex));
		const auto& countries = land.GetCountries();
		const auto types = land.GetMaterialTypes();
		if (cell != nullptr && cell->properties.country < countries.size())
		{
			const auto index = countries[cell->properties.country].materials.at(cell->altitude).indices[1];
			material = index < types.size() ? types[index] : k_NoMaterial;
		}
		if (material == k_NoMaterial)
		{
			material = k_FirstMaterial;
		}
	}
	if (material > k_LastMaterial)
	{
		material = k_NoMaterial;
	}
	const auto& materials = Locator::infoConstants::value().terrainMaterial;
	if (material >= materials.size())
	{
		return k_White;
	}
	const auto dust = glm::min(materials.at(material).tornadoDustColorRGB, glm::uvec3(255));
	return glm::u8vec3(dust);
}

bool GameParticleWorld::IsDryLand(glm::vec3 point) const
{
	if (!Locator::terrainSystem::has_value() || point.x < 0.0f || point.z < 0.0f)
	{
		return false;
	}
	// Land is dry where its cell stands above the sea
	const auto* cell =
	    Locator::terrainSystem::value().FindCell(glm::u16vec2(glm::floor(glm::vec2(point.x, point.z) / k_CellSize)));
	return cell != nullptr && cell->altitude >= k_LowestDryAltitude;
}

std::vector<particles::TornadoCandidate> GameParticleWorld::TornadoCandidates(glm::vec3 foot, float reach) const
{
	return Locator::tornadoSystem::has_value() ? Locator::tornadoSystem::value().Candidates(foot, reach)
	                                           : std::vector<particles::TornadoCandidate> {};
}

void GameParticleWorld::CatchCreature(entt::entity creature)
{
	if (Locator::tornadoSystem::has_value())
	{
		Locator::tornadoSystem::value().CatchCreature(creature);
	}
}

entt::entity GameParticleWorld::TakeFromPile(entt::entity pile, uint32_t amount, float sizeShare)
{
	return Locator::tornadoSystem::has_value() ? Locator::tornadoSystem::value().TakeFromPile(pile, amount, sizeShare)
	                                           : entt::null;
}

std::optional<glm::mat3> GameParticleWorld::Carry(const std::shared_ptr<particles::CarriedObject>& carried)
{
	return Locator::tornadoSystem::has_value() ? Locator::tornadoSystem::value().Carry(carried) : std::nullopt;
}
