/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <entt/fwd.hpp>
#include <glm/fwd.hpp>

namespace openblack
{

namespace graphics
{
class FrameBuffer;
class Mesh;
} // namespace graphics

class OceanInterface
{
public:
	[[nodiscard]] virtual const graphics::FrameBuffer& GetReflectionFramebuffer() const noexcept = 0;
	/// What lies under the sea is drawn at the size of the view, as the game draws it straight into the frame
	virtual void ResizeReflectionFramebuffer(glm::u16vec2 size) = 0;
	[[nodiscard]] virtual graphics::Mesh& GetMesh() const noexcept = 0;
	[[nodiscard]] virtual entt::id_type GetDiffuseTexture() const noexcept = 0;
	[[nodiscard]] virtual entt::id_type GetAlphaTexture() const noexcept = 0;
};
} // namespace openblack
