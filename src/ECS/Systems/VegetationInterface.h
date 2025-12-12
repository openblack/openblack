/*******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <optional>

#include <entt/fwd.hpp>
#include <glm/vec3.hpp>

namespace openblack::ecs::systems
{
class VegetationInterface
{
public:
	virtual void Sway() = 0;
};
} // namespace openblack::ecs::systems
