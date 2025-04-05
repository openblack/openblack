/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "3D/L3DMesh.h"
#include "3D/LandIslandInterface.h"
#include "ECS/Systems/RenderingSystemInterface.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/ShaderManagerInterface.h"
#include "Graphics/VertexBuffer.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"
#include "TechniqueCommon.h"
#include "TechniqueManager.h"

using namespace openblack::graphics;

template <>
void openblack::graphics::renderTechnique<Technique::Footprint>(RendererInterface::DrawSceneDesc desc)
{
	const auto& island = Locator::terrainSystem::value();

	// This dummy draw call is here to make sure that view is cleared if no
	// other draw calls are submitted to view
	bgfx::touch(static_cast<bgfx::ViewId>(desc.viewId));

	// _shaderManager->SetCamera(viewId, *drawDesc.camera); // TODO

	auto view = island.GetOrthoView();
	auto proj = island.GetOrthoProj();
	bgfx::setViewTransform(static_cast<bgfx::ViewId>(desc.viewId), &view, &proj);

	const auto& meshManager = Locator::resources::value().GetMeshes();
	const auto& renderCtx = Locator::rendereringSystem::value().GetContext();
	const auto* footprintShaderInstanced = Locator::shaderManager::value().GetShader("FootprintInstanced");
	for (const auto& [meshId, placers] : renderCtx.instancedDrawDescs)
	{
		auto mesh = meshManager.Handle(meshId);
		if (!mesh->ContainsLandscapeFeature() || mesh->GetFootprints().empty())
		{
			continue;
		}
		const auto& footprint = mesh->GetFootprints()[0];
		footprintShaderInstanced->SetTextureSampler("s_footprint", 0, *footprint.texture);
		footprint.mesh->GetVertexBuffer().Bind();
		bgfx::setInstanceDataBuffer(renderCtx.instanceUniformBuffer, placers.offset, placers.count);
		const uint64_t state = 0u                       //
		                       | BGFX_STATE_WRITE_RGB   //
		                       | BGFX_STATE_WRITE_A     //
		                       | BGFX_STATE_BLEND_ALPHA //
		                       | BGFX_STATE_CULL_CW     //
		                       | BGFX_STATE_MSAA;
		bgfx::setState(state);
		bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), footprintShaderInstanced->GetRawHandle());
	}
}
