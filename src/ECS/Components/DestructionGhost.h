/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <entt/core/fwd.hpp>
#include <glm/mat4x4.hpp>

namespace openblack::ecs::components
{

/// A ghost of a building an effect destroyed, flickering out where it stood for half a second: drawn first into the
/// depth through a scrolling pattern that thins as it goes, then added over itself where that depth was laid
struct DestructionGhost
{
	entt::id_type mesh {0};
	glm::mat4 model {1.0f};
	/// Of the 500 it lasts, in milliseconds of drawing
	int millisecondsLeft {500};
	/// It has been drawn once, so the time drawing it takes runs down from the next frame
	bool shown {false};
};

} // namespace openblack::ecs::components
