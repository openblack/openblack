/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdint>

#include <glm/gtx/transform.hpp>

#include "3D/L3DMesh.h"
#include "3D/SkyInterface.h"
#include "ECS/Systems/RenderingSystemInterface.h"
#include "EngineConfig.h"
#include "Graphics/RendererInterface.h"
#include "Graphics/ShaderManagerInterface.h"
#include "Graphics/VertexBuffer.h"
#include "Locator.h"
#include "Resources/ResourceManager.h"
#include "Resources/ResourcesInterface.h"
#include "TechniqueCommon.h"
#include "TechniqueManager.h"

using namespace openblack::graphics;

template <>
void openblack::graphics::renderTechnique<Technique::Entities>(RendererInterface::DrawSceneDesc desc)
{
	auto& renderer = Locator::rendererInterface::value();
	const auto& meshManager = Locator::resources::value().GetMeshes();
	const auto* objectShaderInstanced = Locator::shaderManager::value().GetShader("ObjectInstanced");
	const auto* objectShaderHeightMapInstanced = Locator::shaderManager::value().GetShader("ObjectHeightMapInstanced");
	const auto* debugShader = Locator::shaderManager::value().GetShader("DebugLine");
	const auto* debugShaderInstanced = Locator::shaderManager::value().GetShader("DebugLineInstanced");

	RendererInterface::L3DMeshSubmitDesc submitDesc = {};
	submitDesc.viewId = desc.viewId;
	submitDesc.program = objectShaderInstanced;
	submitDesc.state = 0u                              //
	                   | BGFX_STATE_WRITE_MASK         //
	                   | BGFX_STATE_DEPTH_TEST_GREATER //
	                   | BGFX_STATE_MSAA               //
	    ;
	const auto& renderCtx = Locator::rendereringSystem::value().GetContext();

	// Instance meshes
	for (const auto& [meshId, placers] : renderCtx.instancedDrawDescs)
	{
		auto mesh = meshManager.Handle(meshId);

		submitDesc.instanceBuffer = &renderCtx.instanceUniformBuffer;
		submitDesc.instanceStart = placers.offset;
		submitDesc.instanceCount = placers.count;
		if (mesh->IsBoned())
		{
			submitDesc.modelMatrices = mesh->GetBoneMatrices().data();
			submitDesc.matrixCount = static_cast<uint8_t>(mesh->GetBoneMatrices().size());
			// TODO(bwrsandman): Get animation frame instead of default
		}
		else
		{
			const static auto identity = glm::mat4(1.0f);
			submitDesc.modelMatrices = &identity;
			submitDesc.matrixCount = 1;
		}
		submitDesc.isSky = false;
		submitDesc.morphWithTerrain = placers.morphWithTerrain;
		submitDesc.program = submitDesc.morphWithTerrain ? objectShaderHeightMapInstanced : objectShaderInstanced;

		// TODO(bwrsandman): choose the correct LOD
		renderer.DrawMesh(*mesh, submitDesc, std::numeric_limits<uint8_t>::max());
	}

	// Debug
	if (desc.viewId == graphics::RenderPass::Main)
	{
		for (const auto& [meshId, placers] : renderCtx.instancedDrawDescs)
		{
			auto mesh = meshManager.Handle(meshId);
			if (!mesh->ContainsLandscapeFeature() || mesh->GetFootprints().empty())
			{
				continue;
			}
		}
		if (renderCtx.boundingBox)
		{
			const auto boundBoxOffset = static_cast<uint32_t>(renderCtx.instanceUniforms.size() / 2);
			const auto boundBoxCount = static_cast<uint32_t>(renderCtx.instanceUniforms.size() / 2);
			renderCtx.boundingBox->GetVertexBuffer().Bind();
			bgfx::setInstanceDataBuffer(renderCtx.instanceUniformBuffer, boundBoxOffset, boundBoxCount);
			bgfx::setState(k_BgfxDefaultStateInvertedZ | BGFX_STATE_PT_LINES);
			bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), debugShaderInstanced->GetRawHandle());
		}
		if (renderCtx.footpaths)
		{
			renderCtx.footpaths->GetVertexBuffer().Bind();
			bgfx::setState(k_BgfxDefaultStateInvertedZ | BGFX_STATE_PT_LINES);
			bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), debugShader->GetRawHandle());
		}
		if (renderCtx.streams)
		{
			renderCtx.streams->GetVertexBuffer().Bind();
			bgfx::setState(k_BgfxDefaultStateInvertedZ | BGFX_STATE_PT_LINES);
			bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), debugShader->GetRawHandle());
		}
	}
}
