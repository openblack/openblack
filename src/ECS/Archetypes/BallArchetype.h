/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <entt/fwd.hpp>
#include <glm/fwd.hpp>

namespace openblack::ecs::archetypes
{
class BallArchetype
{
public:
	/// A football at a point, unturned and at its own size, calling the creatures and animals to it as no one's
	static entt::entity Create(const glm::vec3& position);
	BallArchetype() = delete;
};
} // namespace openblack::ecs::archetypes
