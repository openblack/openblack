/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Graphics/RenderModes.h"
#include "ParticleEffect.h"

/// The particles' sprites as the GPU draws them: one instance each, turned into a quad by the vertex shader
namespace openblack::particles::sprites
{

/// One sprite, five vec4 as the instanced shader reads them
struct SpriteInstance
{
	/// Where the sprite is, and half its width
	glm::vec4 positionHalfWidth;
	/// Half its height, its roll (or yaw when flat), and how far its corners are moved back by its origin, across and up
	glm::vec4 shape;
	/// Its sheet cell: the top left corner in texture space, and the size
	glm::vec4 uv;
	/// Its colour and alpha, 0..1
	glm::vec4 colour;
	/// x: 1 lying flat on the ground, 0 facing the screen
	glm::vec4 flags;
};
static_assert(sizeof(SpriteInstance) == 5 * sizeof(glm::vec4));

/// A drawn sprite is never smaller than this
constexpr float k_MinimumHalfWidth = 1e-4f;

/// The sprite of a drawn atom
[[nodiscard]] SpriteInstance InstanceOf(const Effect::DrawAtom& atom);

/// The game's render mode a sprite or ribbon creator draws in: added to what is behind or blended over it, writing depth or
/// not
[[nodiscard]] graphics::render_modes::Mode RenderMode(const Creator& creator);

/// The four corners of a sprite in the world, as the shader places them, in the order top left, top right, bottom
/// right, bottom left of the sprite. A flat sprite lies on the ground with its top towards -z of its own yaw.
[[nodiscard]] std::array<glm::vec3, 4> Corners(const SpriteInstance& sprite, const glm::vec3& cameraRight,
                                               const glm::vec3& cameraUp);

} // namespace openblack::particles::sprites
