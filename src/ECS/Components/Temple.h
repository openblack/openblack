/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <entt/entity/fwd.hpp>

#include "3D/TempleInteriorInterface.h"
#include "Enums.h"

namespace openblack::ecs::components
{

/// Which of a room's meshes a part is: the game keeps the room and its floor apart, as the main room reflects itself
/// in its floor
enum class TempleInteriorMesh
{
	Room,
	Floor,
	/// The creature's room's waterfall and pools, whose texture slides down them
	Water,
	/// The main room's pool, which the main room draws twice over itself, turned and shimmering
	Pool,
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

/// The way into a temple, Entrance.l3d at the temple's place, which the temple makes and its player clicks the Action
/// button on to go inside
struct TempleEntrance
{
	entt::entity temple;
};
} // namespace openblack::ecs::components
