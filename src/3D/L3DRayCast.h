/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>

#include <glm/vec3.hpp>

namespace openblack::l3d
{
class L3DFile;
}

namespace openblack
{

/// How far along a ray, in lengths of its direction, it first meets a triangle of a mesh, from either side, as the game
/// picks the object under the cursor. All in the mesh's space.
[[nodiscard]] std::optional<float> RayCast(const l3d::L3DFile& mesh, glm::vec3 origin, glm::vec3 direction);

} // namespace openblack
