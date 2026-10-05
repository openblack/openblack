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

#include <array>
#include <optional>

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// Black & White's moon: a half sphere that keeps a place beside the camera, turned to face it, its face lit by the
/// real moon's phase, and a glow about it.
namespace openblack::graphics::moon
{

/// Where the moon stands from the camera at an hour of script time, and how strongly it shows, 0 to 200: it swings on
/// an ellipse through the day, showing only for a few hours either side of midnight. None while it is down.
struct Placement
{
	glm::vec3 offset;
	float alpha;
};
[[nodiscard]] std::optional<Placement> Place(float scriptHour);

/// The phase of the real moon, 0 to 2 pi, by whole days of a seconds since 1970 time
[[nodiscard]] float Phase(int64_t unixTime);

/// The moon's axes in the world: square to the line from the camera, four times the mesh's size
[[nodiscard]] glm::mat3 Basis(const glm::mat4& view, const glm::mat4& inverseView, const glm::vec3& position);

/// The moon mesh's model: the basis tilted a little and turned by the phase, at two thirds the size
[[nodiscard]] glm::mat4 Model(const glm::mat3& basis, const glm::vec3& position, float phase);

/// The glow about the moon: a square of 4000 units on the basis, from part of the atmosphere texture
struct Glow
{
	std::array<glm::vec3, 4> corners;
	std::array<glm::vec2, 4> uvs;
};
[[nodiscard]] Glow MakeGlow(const glm::mat3& basis, const glm::vec3& position);
/// The glow's two triangles
inline constexpr std::array<uint16_t, 6> k_GlowIndices = {0, 1, 3, 3, 2, 0};

/// The glow's colour, a dim copy of the moon's: red a sixth, green a fifth and blue a quarter of it
[[nodiscard]] glm::vec3 GlowColour(const glm::vec3& moonColour);

} // namespace openblack::graphics::moon
