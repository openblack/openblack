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

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"
#include "Magic/DispenserRules.h"

namespace openblack::ecs::components
{

/// A miracle dispenser: a building that floats a one-shot miracle above itself, and makes another some turns after it
/// is taken
struct SpellDispenser
{
	MagicType magicType {MagicType::None};
	magic::DispenserTimer timer;
	/// Its bubble while it has one, and where it made it
	entt::entity orb {entt::null};
	glm::vec3 orbPosition {0.0f};
	/// Its swirl on the land under it, 0 for none
	uint32_t effect {0};
};

/// One of the dispensers the testbed lays out in a grid, cleared away when a scenario asks for none
struct TestbedDispenser
{
};

} // namespace openblack::ecs::components
