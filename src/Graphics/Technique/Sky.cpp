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

#include "3D/SkyInterface.h"
#include "EngineConfig.h"
#include "Graphics/RendererInterface.h"
#include "Graphics/ShaderManagerInterface.h"
#include "Locator.h"
#include "TechniqueCommon.h"
#include "TechniqueManager.h"

using namespace openblack::graphics;

template <>
void openblack::graphics::renderTechnique<Technique::Sky>(RendererInterface::DrawSceneDesc desc)
{
	auto& renderer = Locator::rendererInterface::value();
	const auto* skyShader = Locator::shaderManager::value().GetShader("Sky");

	const auto skyType = Locator::skySystem::value().GetCurrentSkyType();

	const auto modelMatrix = glm::mat4(1.0f);
	const glm::vec4 u_typeAlignment = {skyType, Locator::config::value().skyAlignment + 1.0f, 0.0f, 0.0f};

	skyShader->SetTextureSampler("s_diffuse", 0, Locator::skySystem::value().GetTexture());
	skyShader->SetUniformValue("u_typeAlignment", &u_typeAlignment);

	RendererInterface::L3DMeshSubmitDesc submitDesc = {};
	submitDesc.viewId = desc.viewId;
	submitDesc.program = skyShader;
	submitDesc.state = k_BgfxDefaultStateInvertedZ;
	if (!desc.cullBack)
	{
		submitDesc.state &= ~BGFX_STATE_CULL_MASK;
		submitDesc.state |= BGFX_STATE_CULL_CCW;
	}
	submitDesc.modelMatrices = &modelMatrix;
	submitDesc.matrixCount = 1;
	submitDesc.isSky = true;

	renderer.DrawMesh(Locator::skySystem::value().GetMesh(), submitDesc, 0);
}
