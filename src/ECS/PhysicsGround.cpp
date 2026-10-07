/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "PhysicsGround.h"

#include <cmath>

#include <LNDFile.h>

#include "3D/LandIslandInterface.h"

using namespace openblack;
using namespace openblack::ecs;

namespace
{
constexpr float k_CellSize = 10.0f;
/// The highest a cell of shallow water stands; above this it is dry land
constexpr uint8_t k_HighestWater = 3;
} // namespace

float PhysicsGround::HeightAt(glm::vec2 xz) const
{
	return _land.GetHeightAt(xz);
}

glm::vec3 PhysicsGround::NormalAt(glm::vec2 xz) const
{
	return _land.GetNormalAt(xz);
}

const lnd::LNDCell* PhysicsGround::CellAt(glm::ivec2 cell) const
{
	if (cell.x < 0 || cell.y < 0 || cell.x >= LandIslandInterface::k_MapCellsPerSide ||
	    cell.y >= LandIslandInterface::k_MapCellsPerSide)
	{
		return nullptr;
	}
	return _land.FindCell(glm::u16vec2(cell));
}

std::optional<uint8_t> PhysicsGround::Altitude(glm::ivec2 cell) const
{
	const auto* found = CellAt(cell);
	if (found == nullptr)
	{
		return std::nullopt;
	}
	return found->altitude;
}

bool PhysicsGround::IsLand(glm::vec2 xz) const
{
	const auto* cell = CellAt(glm::ivec2(glm::floor(xz / k_CellSize)));
	return cell != nullptr && cell->properties.hasWater == 0;
}

std::optional<uint8_t> PhysicsGround::CellAltitudeDown(glm::vec2 xz) const
{
	return Altitude(glm::ivec2(glm::floor(xz / k_CellSize)));
}

std::optional<uint8_t> PhysicsGround::CellAltitudeNearest(glm::vec2 xz) const
{
	// Rounded to the nearest, halves to even, as the game's rounding does
	return Altitude(glm::ivec2(static_cast<int32_t>(std::nearbyint(xz.x / k_CellSize)),
	                           static_cast<int32_t>(std::nearbyint(xz.y / k_CellSize))));
}

bool PhysicsGround::IsSeaCell(glm::vec2 xz) const
{
	const auto altitude = CellAltitudeNearest(xz);
	return !altitude.has_value() || *altitude == 0;
}

bool PhysicsGround::IsDryLand(glm::vec2 xz) const
{
	const auto altitude = CellAltitudeDown(xz);
	return altitude.has_value() && *altitude > k_HighestWater;
}

bool PhysicsGround::IsDeepWater(glm::vec2 xz) const
{
	const auto altitude = CellAltitudeNearest(xz);
	return !altitude.has_value() || *altitude < k_HighestWater;
}
