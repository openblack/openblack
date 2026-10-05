/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <vector>

#include <entt/core/hashed_string.hpp>
#include <glm/vec3.hpp>

namespace openblack::ecs::components
{

/// A river: its points in the order the land's script gives them. The river runs from each point to the next.
struct Stream
{
	using Id = int;

	Id id;
	std::vector<glm::vec3> points;
};

/// A stretch of river between two of its points, laid into the land at the first: its bed is blended into the land's
/// colour like a building's footprint, and its channel clears the land's alpha so the sea drawn beneath shows through
/// as the water. Both meshes are a stretch long along their x, turned and stretched to reach the next point.
struct StreamSegment
{
	static constexpr entt::id_type k_ChannelMeshId = entt::hashed_string("river");
	static constexpr entt::id_type k_BedMeshId = entt::hashed_string("river2");
	/// The length of the meshes along their x
	static constexpr float k_MeshLength = 30.0f;
};

} // namespace openblack::ecs::components
