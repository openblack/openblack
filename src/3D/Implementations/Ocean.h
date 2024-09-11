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

#include "3D/OceanInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#pragma message("Locator interface implementations should only be included in Locator.cpp, use interface instead.")
#endif

namespace openblack
{

namespace graphics
{
class FrameBuffer;
class Mesh;
} // namespace graphics

class Ocean final: public OceanInterface
{
public:
	// Oddly enough the water texture is labeled Sky
	static constexpr entt::hashed_string k_DiffuseTextureId = entt::hashed_string("raw/Sky");
	static constexpr entt::hashed_string k_AlphaTextureId = entt::hashed_string("raw/Skya");

	Ocean() noexcept;
	~Ocean() noexcept;

	[[nodiscard]] graphics::FrameBuffer& GetReflectionFramebuffer() const noexcept override { return *_reflectionFrameBuffer; }
	[[nodiscard]] graphics::Mesh& GetMesh() const noexcept override { return *_mesh; }
	[[nodiscard]] entt::id_type GetDiffuseTexture() const noexcept override { return k_DiffuseTextureId; }
	[[nodiscard]] entt::id_type GetAlphaTexture() const noexcept override { return k_AlphaTextureId; }

private:
	void CreateMesh();

	std::unique_ptr<graphics::Mesh> _mesh;
	std::unique_ptr<graphics::FrameBuffer> _reflectionFrameBuffer;
};

} // namespace openblack
