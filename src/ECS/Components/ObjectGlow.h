/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <glm/gtc/type_precision.hpp>

namespace openblack::ecs::components
{

/// A colour added to how an object is drawn, after its texture and light, as the heal's chakra lights the people it heals.
/// What lights it sets it each step; once nothing has for a few turns it goes.
struct ObjectGlow
{
	glm::u8vec3 rgb {0};
	/// Game turns since it was last set
	uint8_t turnsUnset {0};

	/// As one number, 0xRRGGBB
	[[nodiscard]] uint32_t Packed() const
	{
		return (static_cast<uint32_t>(rgb.r) << 16u) | (static_cast<uint32_t>(rgb.g) << 8u) | rgb.b;
	}
};

} // namespace openblack::ecs::components
