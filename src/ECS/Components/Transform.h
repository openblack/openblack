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

#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

namespace openblack::ecs::components
{

struct Transform
{
	glm::vec3 position;
	glm::mat3 rotation;
	// TODO(bwrsandman): Change to float. For all intents and purposes, transform scales uniformly.
	glm::vec3 scale;
};

inline std::optional<glm::vec3> GetTransformPosition(const Transform& transform)
{
	return transform.position;
};

} // namespace openblack::ecs::components
