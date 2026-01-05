/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include "3D/LandBlock.h"
#include "3D/LandIslandInterface.h"
#include "3D/SkyInterface.h"
#include "EngineConfig.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/Mesh.h"
#include "Graphics/ShaderManagerInterface.h"
#include "Graphics/VertexBuffer.h"
#include "Locator.h"
#include "Resources/ResourceManager.h"
#include "Resources/ResourcesInterface.h"
#include "TechniqueCommon.h"
#include "TechniqueManager.h"

using namespace openblack::graphics;

template <>
void openblack::graphics::renderTechnique<Technique::Island>(RendererInterface::DrawSceneDesc desc)
{
	const auto* terrainShader = Locator::shaderManager::value().GetShader("Terrain");
	const auto skyType = Locator::skySystem::value().GetCurrentSkyType();

	auto& island = Locator::terrainSystem::value();
	auto islandExtent = glm::vec4(island.GetExtent().minimum, island.GetExtent().maximum);

	auto texture = Locator::resources::value().GetTextures().Handle(LandIslandInterface::k_SmallBumpTextureId);
	const glm::vec4 u_skyAndBump = {skyType, desc.bumpMapStrength, desc.smallBumpMapStrength, 0.0f};

	terrainShader->SetTextureSampler("s0_materials", 0, island.GetAlbedoArray());
	terrainShader->SetTextureSampler("s1_bump", 1, island.GetBump());
	terrainShader->SetTextureSampler("s2_smallBump", 2, *texture);
	terrainShader->SetTextureSampler("s3_footprints", 3, island.GetFootprintFramebuffer().GetColorAttachment());

	terrainShader->SetUniformValue("u_skyAndBump", &u_skyAndBump);
	terrainShader->SetUniformValue("u_islandExtent", &islandExtent);

	// clang-format off
    constexpr auto k_DefaultState = 0u
        | BGFX_STATE_WRITE_MASK
        | BGFX_STATE_DEPTH_TEST_GREATER
        | BGFX_STATE_BLEND_ALPHA
        | BGFX_STATE_MSAA
    ;

    constexpr auto k_Discard = 0u
        | BGFX_DISCARD_INSTANCE_DATA
        | BGFX_DISCARD_INDEX_BUFFER
        | BGFX_DISCARD_TRANSFORM
        | BGFX_DISCARD_VERTEX_STREAMS
        | BGFX_DISCARD_STATE
    ;
	// clang-format on

	for (const auto& block : island.GetBlocks())
	{
		// pack uniforms
		const glm::vec4 mapPositionAndSize = glm::vec4(block.GetMapPosition(), 160.0f, 160.0f);
		terrainShader->SetUniformValue("u_blockPositionAndSize", &mapPositionAndSize);

		block.GetMesh().GetVertexBuffer().Bind();

		bgfx::setState(k_DefaultState | (desc.cullBack ? BGFX_STATE_CULL_CCW : BGFX_STATE_CULL_CW), 0);
		bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), terrainShader->GetRawHandle(), 0, k_Discard);
	}
	bgfx::discard(BGFX_DISCARD_BINDINGS);
}
