/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

#include <array>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// The villagers' ground blobs: a short dark shadow stretched from each foot over the land, away from a light low in
/// the south east, fading as it goes.
namespace openblack::graphics::ground_blobs
{

/// The bones whose origins are a villager's feet
inline constexpr std::array<size_t, 2> k_FootBones = {21, 18};
/// Things this low, in the sea, have none
inline constexpr float k_LowestHeight = 0.2f;
/// How far over the land a blob lies
inline constexpr float k_Lift = 0.2f;

/// A blob's quad: from just behind the foot, across it, out to its far end
struct Quad
{
	std::array<glm::vec3, 4> corners;
};
/// The texture coordinates of a quad's corners, and their opacity: whole at the foot, none at the far end
inline constexpr std::array<glm::vec2, 4> k_Uvs = {glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f),
                                                   glm::vec2(0.0f, 1.0f)};
inline constexpr std::array<float, 4> k_Opacity = {1.0f, 1.0f, 0.0f, 0.0f};
/// The quad's two triangles
inline constexpr std::array<uint16_t, 6> k_Indices = {0, 1, 2, 0, 2, 3};

/// How far and which way the shadows fall for a thing of a scale standing on land of a normal: the light's offset laid
/// flat on the land
[[nodiscard]] glm::vec3 Fall(const glm::vec3& landNormal, float scale);
/// A blob from a foot, falling by `fall`
[[nodiscard]] Quad MakeQuad(const glm::vec3& foot, const glm::vec3& fall);
/// The two blobs of a pair of feet: each falls by the shadow's fall and leans halfway towards the other foot
[[nodiscard]] std::array<Quad, 2> Feet(const glm::vec3& first, const glm::vec3& second, const glm::vec3& fall);

} // namespace openblack::graphics::ground_blobs
