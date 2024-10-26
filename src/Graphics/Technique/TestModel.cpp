/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdint>

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
void openblack::graphics::renderTechnique<Technique::TestModel>(RendererInterface::DrawSceneDesc desc)
{
	auto& renderer = Locator::rendererInterface::value();
	const auto& meshManager = Locator::resources::value().GetMeshes();

	RendererInterface::L3DMeshSubmitDesc submitDesc = {};
	submitDesc.viewId = desc.viewId;
	submitDesc.program = Locator::shaderManager::value().GetShader("Object");
	// clang-format off
    submitDesc.state = 0u
        | BGFX_STATE_WRITE_MASK
        | BGFX_STATE_DEPTH_TEST_GREATER
        | BGFX_STATE_CULL_CCW
        | BGFX_STATE_MSAA
    ;
	// clang-format on
	const auto& mesh = meshManager.Handle(entt::hashed_string("coffre"));
	const auto& testAnimation = Locator::resources::value().GetAnimations().Handle(entt::hashed_string("coffre"));
	const std::vector<uint32_t>& boneParents = mesh->GetBoneParents();
	auto bones = testAnimation->GetBoneMatrices(desc.time);
	for (uint32_t i = 0; i < bones.size(); ++i)
	{
		if (boneParents[i] != std::numeric_limits<uint32_t>::max())
		{
			bones[i] = bones[boneParents[i]] * bones[i];
		}
	}
	submitDesc.modelMatrices = bones.data();
	submitDesc.matrixCount = static_cast<uint8_t>(bones.size());
	submitDesc.isSky = false;
	renderer.DrawMesh(*mesh, submitDesc, 0);
}
