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

#include <optional>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Physics/Ground.h"

namespace openblack
{
class LandIslandInterface;
namespace lnd
{
struct LNDCell;
}
} // namespace openblack

namespace openblack::ecs
{

/// The island's land as the physics feels it: its height and the flat normal of its triangles, and which of its cells
/// are open sea
class PhysicsGround final: public physics::Ground
{
public:
	explicit PhysicsGround(const LandIslandInterface& land)
	    : _land(land)
	{
	}

	[[nodiscard]] float HeightAt(glm::vec2 xz) const override;
	[[nodiscard]] glm::vec3 NormalAt(glm::vec2 xz) const override;
	[[nodiscard]] bool IsSeaCell(glm::vec2 xz) const override;

	/// The altitude of the cell a point is in (its coordinates rounded down to the cell), none off the map
	[[nodiscard]] std::optional<uint8_t> CellAltitudeDown(glm::vec2 xz) const;
	/// The altitude of the cell nearest a point (its coordinates rounded to the nearest cell), none off the map
	[[nodiscard]] std::optional<uint8_t> CellAltitudeNearest(glm::vec2 xz) const;
	/// Dry land: the cell a point is in stands more than 3 above the sea
	[[nodiscard]] bool IsDryLand(glm::vec2 xz) const;
	/// Land: the cell a point is in has no water in it; off the map is not land
	[[nodiscard]] bool IsLand(glm::vec2 xz) const;
	/// Where something landing hits deep water: the nearest cell is off the map, missing, or below 3
	[[nodiscard]] bool IsDeepWater(glm::vec2 xz) const;

private:
	[[nodiscard]] const lnd::LNDCell* CellAt(glm::ivec2 cell) const;
	[[nodiscard]] std::optional<uint8_t> Altitude(glm::ivec2 cell) const;

	const LandIslandInterface& _land;
};

} // namespace openblack::ecs
