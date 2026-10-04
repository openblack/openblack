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

#include <span>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace openblack::graphics
{

/// The light the god hand carries over the land at night.
struct HandLight
{
	/// Brightnesses across each side of the map, one per vertex
	static constexpr uint16_t k_Size = 12;
	/// The land's vertices are a cell apart
	static constexpr float k_Spacing = 10.0f;

	/// Where the map's first brightness lies in the world's x and z
	[[nodiscard]] static glm::vec2 GetOrigin(glm::vec3 handPosition);
	/// How strongly the hand lights the land at a sky type, from 0 at full night to 2 in full day. The game brings its
	/// light up over the last fifteenth of the ambient light's dimming towards night; here that is the dusk's turning
	/// to night.
	[[nodiscard]] static float GetStrength(float skyType);
	/// The brightness the hand gives the land at a point of the world's x and z, 0 to 1.
	[[nodiscard]] static float GetBrightness(std::span<const uint8_t> map, glm::vec3 handPosition, glm::vec2 point);
};

} // namespace openblack::graphics
