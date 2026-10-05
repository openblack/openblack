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

#include "ECS/Systems/TempleExteriorSystemInterface.h"
#include "Enums.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::components
{
struct Mesh;
struct TempleExterior;
} // namespace openblack::ecs::components

namespace openblack::ecs::systems
{

class TempleExteriorSystem final: public TempleExteriorSystemInterface
{
public:
	void UpdateTurn() override;

private:
	/// fn_00882B10: blends the temple's own mesh from the four about its size and alignment, and its texture between
	/// the two looks about its alignment, of its player's set
	static void Morph(entt::entity entity, components::Mesh& mesh, const components::TempleExterior& exterior,
	                  PlayerNames owner);
};

} // namespace openblack::ecs::systems
