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

#include <entt/entity/entity.hpp>

#include "Physics/DamageMesh.h"

namespace openblack::ecs::components
{

/// A building a rock has broken: its model as loose triangles in the world, drawn in place of its whole model
struct BuildingDamage
{
	physics::damage::Mesh mesh;
	/// The rock that last broke it: struck again by the same rock hard enough, the two pass through each other
	entt::entity lastHitter {entt::null};
	/// The life its repair starts from, set at each breaking blow; none before the first
	std::optional<float> repairStart;
	/// The model it is drawn with, made from the broken triangles
	entt::id_type drawMesh {0};
	/// Counts the models made for it, each named apart
	uint32_t version {0};
};

/// A piece broken off a building, flying or lying where it fell until its time runs out
struct BuildingPiece
{
	/// Its triangles about its own centre
	physics::damage::Mesh mesh;
	/// The building it came from, none once that is gone or the piece has come to rest
	entt::entity parent {entt::null};
	/// The model of the building it came from, whose skins it is drawn with
	entt::id_type sourceMesh {0};
	/// Game turns before it goes
	uint32_t turnsLeft {0};
	/// The model it is drawn with
	entt::id_type drawMesh {0};
};

} // namespace openblack::ecs::components
