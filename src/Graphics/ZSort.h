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

#include <bit>

#include <glm/vec3.hpp>

/// The order the game draws the things that blend in the world: all of them together, the farthest from the camera
/// first, whatever they are. Models that fade or have alpha, sprites, mists and clouds, and the hand all go through it.
namespace openblack::graphics::zsort
{

/// How far a point is from the camera for the sort: its distance squared, in single precision as the game works it
[[nodiscard]] inline float Key(const glm::vec3& point, const glm::vec3& camera) noexcept
{
	const auto d = point - camera;
	return ((d.x * d.x) + (d.y * d.y)) + (d.z * d.z);
}

/// The key as a bgfx depth for a view sorted by depth, the greatest first. A float that isn't negative orders the same
/// as its bits.
[[nodiscard]] inline uint32_t Depth(const glm::vec3& point, const glm::vec3& camera) noexcept
{
	return std::bit_cast<uint32_t>(Key(point, camera));
}

} // namespace openblack::graphics::zsort
