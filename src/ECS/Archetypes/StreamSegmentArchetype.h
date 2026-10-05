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
class StreamSegmentArchetype
{
public:
	/// A stretch of river from one of its points to the next
	static entt::entity Create(const glm::vec3& from, const glm::vec3& to);
	StreamSegmentArchetype() = delete;
};
} // namespace openblack::ecs::archetypes
