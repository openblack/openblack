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

namespace openblack::ecs::components
{

/// The football: a thing lying about, of the ball's own table row, that the hand and creatures play with as a toy
struct Ball
{
	/// Where it was last kicked at
	std::optional<glm::vec3> destination;
};

} // namespace openblack::ecs::components
