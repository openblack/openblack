/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "3D/MapCoords.h"

/// How a villager finds its way round a fire: the point to make for to pass a circle, all in map units as the game
/// works it out. Pure rules, tested without the game.
namespace openblack::fire
{

/// The result of looking for a way round a circle
struct ViaPoint
{
	/// Which way round, as the angle turned; 0 when the way needs no detour or the detour is no good
	float angle {0.0f};
	map_coords::MapCoords point {};
	bool detour {false};
	/// The start, or the end once a detour was needed, lies inside the circle
	bool inside {false};
};

/// The point to make for to pass a circle of a radius, in metres, by a margin on the way from one point to another, on
/// the side a preference gives when it has one (the angle a last detour turned), else on the side the way leans to.
/// There is no detour when the way clears the circle, when it would be longer than the way itself, or when the start
/// is inside.
[[nodiscard]] ViaPoint GetViaPoint(const map_coords::MapCoords& from, const map_coords::MapCoords& to,
                                   const map_coords::MapCoords& centre, float radius, float margin, float side);

} // namespace openblack::fire
