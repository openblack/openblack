/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The renderer is one of the locator's implementations
#define LOCATOR_IMPLEMENTATIONS

#include <cstring>

#include <algorithm>
#include <span>

#include <bgfx/bgfx.h>

#include "3D/LandIslandInterface.h"
#include "3D/TempleInteriorInterface.h"
#include "Camera/Camera.h"
#include "Creature/LeashRope.h"
#include "ECS/Components/CreatureLeash.h"
#include "ECS/Registry.h"
#include "Graphics/GraphicsHandleBgfx.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/ZSort.h"
#include "Locator.h"
#include "Renderer.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::graphics;

namespace
{
struct Vertex
{
	glm::vec3 position;
	glm::vec2 uv;
	uint32_t colour;
};

const bgfx::VertexLayout& Layout()
{
	static const auto k_Layout = [] {
		bgfx::VertexLayout layout;
		layout.begin()
		    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
		    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
		    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
		    .end();
		return layout;
	}();
	return k_Layout;
}

/// White, or black for the shadow, at an opacity
uint32_t Abgr(float alpha, bool black)
{
	const auto a = static_cast<uint32_t>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f + 0.5f);
	return (a << 24u) | (black ? 0u : 0xFFFFFFu);
}

/// Fills a transient buffer with a ribbon's corners and its triangles; false when there is no room this frame
bool Fill(std::span<const leash_rope::RibbonVertex> corners, bool black, bgfx::TransientVertexBuffer& vertices,
          bgfx::TransientIndexBuffer& indices)
{
	const auto vertexCount = static_cast<uint32_t>(corners.size());
	const auto triangles = leash_rope::RibbonIndices();
	const auto indexCount = static_cast<uint32_t>(triangles.size());
	if (bgfx::getAvailTransientVertexBuffer(vertexCount, Layout()) < vertexCount ||
	    bgfx::getAvailTransientIndexBuffer(indexCount) < indexCount)
	{
		return false;
	}
	bgfx::allocTransientVertexBuffer(&vertices, vertexCount, Layout());
	bgfx::allocTransientIndexBuffer(&indices, indexCount);
	auto out = std::span(reinterpret_cast<Vertex*>(vertices.data), vertexCount);
	for (size_t i = 0; i < corners.size(); ++i)
	{
		out[i] = {corners[i].position, corners[i].uv, Abgr(corners[i].alpha, black)};
	}
	std::memcpy(indices.data, triangles.data(), triangles.size() * sizeof(uint16_t));
	return true;
}

/// The land's light the ropes take: halved in the sea's reflection, as everything's is
float LandLightScale(RenderPass viewId)
{
	constexpr float k_ReflectionLight = 0.5f;
	return viewId == RenderPass::Reflection || viewId == RenderPass::ReflectionTranslucent ? k_ReflectionLight : 1.0f;
}
} // namespace

void Renderer::DrawLeashes(const DrawSceneDesc& desc) const
{
	using ecs::components::CreatureLeash;
	if (!Locator::terrainSystem::has_value() || (Locator::temple::has_value() && Locator::temple::value().Active()))
	{
		return;
	}
	const auto& textures = Locator::resources::value().GetTextures();
	const bool textured = textures.Contains(CreatureLeash::k_TextureId) && textures.Contains(CreatureLeash::k_AlphaTextureId);
	if (!textured && !_whiteTexture.has_value())
	{
		return;
	}
	const auto& island = Locator::terrainSystem::value();
	const auto ground = [&island](glm::vec2 point) { return island.GetHeightAt(point); };
	const auto eye = desc.camera->GetOrigin();
	const auto extent = island.GetExtent();
	const auto islandExtent = glm::vec4(extent.minimum, extent.maximum);
	const glm::vec4 u_landLight {_landLightTexture.has_value() ? 1.0f : 0.0f, LandLightScale(desc.viewId), 0.0f, 0.0f};
	const auto translucent = static_cast<bgfx::ViewId>(TranslucentPassOf(desc.viewId));
	const auto* rope = _shaderManager->GetShader("Leash");
	const auto* shadow = _shaderManager->GetShader("WorldTextured");
	const auto bind = [&](const ShaderProgram& program) {
		if (textured)
		{
			program.SetTextureSampler("s_diffuse", 0, *textures.Handle(CreatureLeash::k_TextureId));
			program.SetTextureSampler("s_alpha", 1, *textures.Handle(CreatureLeash::k_AlphaTextureId));
		}
		else
		{
			program.SetTextureSampler("s_diffuse", 0, *_whiteTexture);
			program.SetTextureSampler("s_alpha", 1, *_whiteTexture);
		}
	};

	desc.entities.Each<const CreatureLeash>([&](const CreatureLeash& leashes) {
		if (!leashes.drawn || !leashes.worn.has_value() || !leashes.worn->ropeStarted)
		{
			return;
		}
		const auto ribbon = leash_rope::BuildRibbon(leashes.worn->rope, eye, ground);
		const auto middle = leash_rope::Point(leashes.worn->rope, leash_rope::k_PointCount / 2);

		// The shadow first, flat on the land in the main view only, the reflection having no land to lie on
		bgfx::TransientVertexBuffer vertices;
		bgfx::TransientIndexBuffer indices;
		if (desc.viewId == RenderPass::Main && Fill(ribbon.shadow, true, vertices, indices))
		{
			bind(*shadow);
			bgfx::setVertexBuffer(0, &vertices);
			bgfx::setIndexBuffer(&indices);
			// Blended over the land, tested against depth but leaving none, both sides
			bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_BLEND_ALPHA | BGFX_STATE_MSAA);
			shadow->Submit(static_cast<uint16_t>(desc.viewId));
		}

		// Then the rope, lit by the land, blended by the texture's alpha among the translucent things
		if (Fill(ribbon.rope, false, vertices, indices))
		{
			bind(*rope);
			rope->SetTextureSampler("s_landLuminosity", 6, GetLandLuminosity());
			rope->SetTextureSampler("s_landLight", 7, GetLandLightTexture());
			rope->SetTextureSampler("s_landColour", 8, GetLandColour());
			rope->SetUniformValue("u_islandExtent", &islandExtent);
			rope->SetUniformValue("u_landLight", &u_landLight);
			rope->SetUniformValue("u_haze", &_haze[0]);
			rope->SetUniformValue("u_hazeColour", &_haze[1]);
			bgfx::setVertexBuffer(0, &vertices);
			bgfx::setIndexBuffer(&indices);
			bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_MSAA |
			               BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA));
			rope->Submit(static_cast<uint16_t>(translucent), zsort::Depth(middle, eye));
		}
	});
}
