/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <glm/vec3.hpp>

namespace openblack::graphics
{

/// The shadows of trees, rocks, buildings and the island's other features on the land. Black & White bakes them into
/// its land textures: it casts each object's triangles from a fixed sun onto the
/// flat plane through the object's origin, wherever the land under it lies, through its textures' masks. The shadows
/// cover a texel's eight coverage samples together, so where shadows overlap the land is no darker, and a fully
/// covered texel is half as bright. Here the objects are cast the same way into a texture over the whole island, which
/// the land is darkened by.
struct ObjectShadows
{
	/// The game's default sun, from the object's origin: so far away that its shadows all fall the same way, as long
	/// along the land's x and z as the object is high
	static constexpr glm::vec3 k_Sun {-500000.0f, 500000.0f, -500000.0f};
	/// Under a fully covered texel the land's colour is multiplied by (255 - 255 / 2) / 256
	static constexpr float k_MaxDarkness = 0.5f;

	/// Where a point of an object with its origin at origin falls on the plane through the origin. Points
	/// under the origin fall straight down.
	[[nodiscard]] static glm::vec3 Project(glm::vec3 point, glm::vec3 origin);
};

} // namespace openblack::graphics
