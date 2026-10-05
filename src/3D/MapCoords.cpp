/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "MapCoords.h"

#include "3D/LandIslandInterface.h"

using namespace openblack;

namespace
{
/// LH3DIsland::GetAltitude 0x803090 at the MapCoords' own (truncated) position
float GroundAt(const LandIslandInterface& island, const map_coords::MapCoords& coords)
{
	return static_cast<float>(island.GetAltitude(coords.x, coords.z));
}
} // namespace

map_coords::MapCoords map_coords::FromWorld(const LandIslandInterface& island, glm::vec3 point)
{
	MapCoords coords {ToFixed(point.x), ToFixed(point.z), 0.0f};
	// call 0x803090; fld [edi + 4]; fsub st(1) (0x603371..0x60337C)
	coords.altitude = point.y - GroundAt(island, coords);
	return coords;
}

glm::vec3 map_coords::ToWorld(const LandIslandInterface& island, const MapCoords& coords)
{
	return {ToMetres(coords.x), GroundAt(island, coords) + coords.altitude, ToMetres(coords.z)};
}
