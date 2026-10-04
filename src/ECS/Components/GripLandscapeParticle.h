/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <glm/vec3.hpp>

namespace openblack::ecs::components
{

/// One sprite of the puff of dust thrown up where the hand grips the land
struct GripLandscapeParticle
{
	/// The puff's centre, which the sprites spread out from as it grows
	glm::vec3 centre;
	/// Where the sprite sits in a puff of unit scale
	glm::vec3 offset;
	/// Seconds since the puff appeared
	float age = 0.0f;
	/// Animation frame, in frames since the start of the sprite sheet
	float frame = 0.0f;
};

} // namespace openblack::ecs::components
