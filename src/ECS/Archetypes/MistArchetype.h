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

#include <entt/fwd.hpp>
#include <glm/fwd.hpp>

namespace openblack::ecs::archetypes
{
class MistArchetype
{
public:
	/// A puff of mist `altitude` above the land at `position`. An `edgeShrink` other than 1 makes it shrink edge on
	/// that much.
	static entt::entity Create(const glm::vec3& position, float altitude, uint32_t colour, float size, float edgeShrink);
	MistArchetype() = delete;
};
} // namespace openblack::ecs::archetypes
