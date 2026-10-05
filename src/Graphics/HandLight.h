/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace openblack::graphics
{

/// The light the god hand carries over the land at night: it stamps its image of brightness into the land's luminosity
/// (see village_lights) centred on the hand.
struct HandLight
{
	/// Brightnesses across each side of its image, a cell apart
	static constexpr uint16_t k_Size = 12;
	static constexpr float k_Spacing = 10.0f;

	/// Where the image's first brightness lies in the world's x and z
	[[nodiscard]] static glm::vec2 GetOrigin(glm::vec3 handPosition);
	/// How strongly the hand lights the world, 0 to 1, by the land's colour of the frame (0xRRGGBB): its light comes up
	/// as the mean of the land's channels falls from 120 to 105, and is full below.
	[[nodiscard]] static float GetStrength(uint32_t landColour);
};

} // namespace openblack::graphics
