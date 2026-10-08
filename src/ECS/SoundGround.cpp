/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "SoundGround.h"

#include <cmath>

#include <LNDFile.h>

#include "3D/LandIslandInterface.h"
#include "ECS/Systems/SnowSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"

using namespace openblack;

std::optional<creature_audio::Ground> ecs::sound_ground::At(const glm::vec3& position)
{
	if (!Locator::terrainSystem::has_value())
	{
		return std::nullopt;
	}
	const auto& land = Locator::terrainSystem::value();
	const auto cellX = static_cast<int32_t>(std::floor(position.x / LandIslandInterface::k_CellSize));
	const auto cellZ = static_cast<int32_t>(std::floor(position.z / LandIslandInterface::k_CellSize));
	if (cellX < 0 || cellZ < 0 || cellX >= LandIslandInterface::k_MapCellsPerSide ||
	    cellZ >= LandIslandInterface::k_MapCellsPerSide)
	{
		return std::nullopt;
	}
	const auto* cell = land.FindCell({static_cast<uint16_t>(cellX), static_cast<uint16_t>(cellZ)});
	if (cell == nullptr)
	{
		return std::nullopt;
	}
	if (cell->properties.hasWater != 0)
	{
		return creature_audio::Ground {.water = true, .materialSurface = 0};
	}

	// The second of the two materials the cell's country blends at its altitude
	std::optional<uint16_t> materialType;
	const auto& countries = land.GetCountries();
	const auto types = land.GetMaterialTypes();
	if (cell->properties.country < countries.size())
	{
		const auto index = countries[cell->properties.country].materials.at(cell->altitude).indices[1];
		if (index < types.size())
		{
			materialType = types[index];
		}
	}
	const auto snow = Locator::snowSystem::has_value() ? Locator::snowSystem::value().GetDepth({position.x, position.z}) : 0.0f;
	const auto material = creature_audio::TerrainMaterial(materialType, snow);

	int32_t surface = 0;
	if (Locator::infoConstants::has_value())
	{
		const auto& materials = Locator::infoConstants::value().terrainMaterial;
		if (material < materials.size())
		{
			surface = static_cast<int32_t>(materials.at(material).surfaceSound);
		}
	}
	return creature_audio::Ground {.water = false, .materialSurface = surface};
}
