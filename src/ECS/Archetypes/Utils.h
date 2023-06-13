/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <tuple>

#include <glm/vec2.hpp>

namespace openblack
{
enum class MeshId : uint32_t;
}

namespace openblack::ecs::components
{
struct Transform;
}

namespace openblack::ecs::archetypes
{
std::pair<glm::vec2, float> GetFixedObstacleBoundingCircle(MeshId meshId, const components::Transform& transform);
}
