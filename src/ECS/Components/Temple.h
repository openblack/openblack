/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "3D/TempleInteriorInterface.h"
#include "Enums.h"

namespace openblack::ecs::components
{

/// Which of a room's meshes a part is: InnerRoom keeps the room and its floor apart, as the main room reflects itself
/// in its floor
enum class TempleInteriorMesh
{
	Room,
	Floor,
	/// The creature's room's waterfall and pools, whose texture slides down them
	Water,
	Other,
};

struct TempleInteriorPart
{
	TempleRoom room;
	TempleInteriorMesh mesh {TempleInteriorMesh::Other};
};

struct Temple
{
	PlayerNames owner;
};
} // namespace openblack::ecs::components
