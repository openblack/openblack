/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Graphics/RenderPass.h"

#include <glm/vec4.hpp>

#include <memory>

namespace openblack
{

namespace graphics
{
class FrameBuffer;
class Mesh;
class ShaderProgram;
class Texture2D;
} // namespace graphics

class Water
{
public:
	Water();
	~Water() = default;

	[[nodiscard]] glm::vec4 GetReflectionPlane() const { return glm::vec4(0.0f, 1.0f, 0.0f, 0.0f); };
	[[nodiscard]] graphics::FrameBuffer& GetFrameBuffer() const;

private:
	friend class Renderer;

	void createMesh();

	std::unique_ptr<graphics::Mesh> _mesh;
	std::unique_ptr<graphics::FrameBuffer> _reflectionFrameBuffer;
	std::unique_ptr<graphics::Texture2D> _texture;
};

} // namespace openblack
