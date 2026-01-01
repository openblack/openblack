/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <glm/vec2.hpp>

namespace openblack::ecs::components
{

struct Swayable
{
	// Flag to indicate that the object is swayed by the player's hand
	bool swayedByHand;
	// Time offset for animation. This randomises the swaying for a more natural effect
	float swayOffset;
	// Timer for animation
	float swayTime;
	// Sway strength base
	float swayStrengthBase;
	// Current sway strength
	float swayStrength;
	// Normalized sway direction (x,z)
	glm::vec2 swayDirection;
};

} // namespace openblack::ecs::components
