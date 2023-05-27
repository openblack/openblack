/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <memory>

#include <entt/core/hashed_string.hpp>
#include <glm/vec4.hpp>

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
	// Oddly enough the water texture is labeled Sky
	static constexpr entt::hashed_string k_DiffuseTextureId = entt::hashed_string("raw/Sky");
	static constexpr entt::hashed_string k_AlphaTextureId = entt::hashed_string("raw/Skya");

	Water();
	~Water();

	[[nodiscard]] graphics::FrameBuffer& GetFrameBuffer() const;

private:
	friend class Renderer;

	void CreateMesh();

	std::unique_ptr<graphics::Mesh> _mesh;
	std::unique_ptr<graphics::FrameBuffer> _reflectionFrameBuffer;
};

} // namespace openblack
