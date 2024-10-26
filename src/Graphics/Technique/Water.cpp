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

#include "3D/OceanInterface.h"
#include "3D/SkyInterface.h"
#include "EngineConfig.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/IndexBuffer.h"
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
void openblack::graphics::renderTechnique<Technique::Water>(RendererInterface::DrawSceneDesc desc)
{
	const auto* waterShader = Locator::shaderManager::value().GetShader("Water");
	const auto skyType = Locator::skySystem::value().GetCurrentSkyType();

	const auto& ocean = Locator::oceanSystem::value();
	const auto& mesh = ocean.GetMesh();
	mesh.GetIndexBuffer().Bind(mesh.GetIndexBuffer().GetCount(), 0);
	mesh.GetVertexBuffer().Bind();
	bgfx::setState(k_BgfxDefaultStateInvertedZ);
	auto diffuse = Locator::resources::value().GetTextures().Handle(ocean.GetDiffuseTexture());
	auto alpha = Locator::resources::value().GetTextures().Handle(ocean.GetAlphaTexture());
	waterShader->SetTextureSampler("s_diffuse", 0, *diffuse);
	waterShader->SetTextureSampler("s_alpha", 1, *alpha);
	waterShader->SetTextureSampler("s_reflection", 2, ocean.GetReflectionFramebuffer().GetColorAttachment());
	const glm::vec4 u_sky = {skyType, 0.0f, 0.0f, 0.0f};
	waterShader->SetUniformValue("u_sky", &u_sky); // fs
	bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), waterShader->GetRawHandle());
}
