/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

// The physical shield's domes: the solid shield model, lit where it stands, its alpha textured parts added over what is
// behind

#include <cstdint>

#include <limits>
#include <vector>

#include <bgfx/bgfx.h>
#include <glm/gtx/transform.hpp>

#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
#include "Camera/Camera.h"
#include "ECS/Systems/MagicShieldSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "Graphics/RenderModes.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/ZSort.h"
#include "Locator.h"
#include "Renderer.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::graphics;

void Renderer::DrawShieldDomes(const DrawSceneDesc& desc) const
{
	const bool reflection = desc.viewId == RenderPass::Reflection;
	if ((desc.viewId != RenderPass::Main && !reflection) || !Locator::magicShieldSystem::has_value() ||
	    !Locator::time::has_value())
	{
		return;
	}
	const auto domes = Locator::magicShieldSystem::value().GetDomes(Locator::time::value().GetTurnFraction());
	if (domes.empty())
	{
		return;
	}
	const auto& meshes = Locator::resources::value().GetMeshes();
	const auto origin = desc.camera->GetOrigin();
	for (const auto& dome : domes)
	{
		if (!meshes.Contains(dome.mesh))
		{
			continue;
		}
		const auto mesh = meshes.Handle(dome.mesh);
		// Its solid shape isn't drawn
		std::vector<uint32_t> hidden;
		const auto& subMeshes = mesh->GetSubMeshes();
		for (uint32_t i = 0; i < subMeshes.size(); ++i)
		{
			if (subMeshes[i]->IsPhysics())
			{
				hidden.push_back(i);
			}
		}
		const glm::mat4 model = glm::translate(dome.position) * glm::rotate(dome.angle, glm::vec3(0.0f, 1.0f, 0.0f)) *
		                        glm::scale(glm::vec3(dome.scale));
		L3DMeshSubmitDesc submitDesc = {};
		submitDesc.viewId = TranslucentPassOf(desc.viewId);
		submitDesc.program = _shaderManager->GetShader("Object");
		submitDesc.modelMatrices = &model;
		submitDesc.matrixCount = 1;
		submitDesc.sortDepth = zsort::Depth(dome.position, origin);
		submitDesc.mirrored = reflection;
		submitDesc.hiddenSubMeshes = hidden;
		// Lit by the land's light where it stands, its materials as made, the alpha textured ones added over what is
		// behind without writing depth. The game fades only its particle effect by the shield's strength, not the dome.
		submitDesc.state =
		    BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_MSAA;
		submitDesc.useMaterialBlending = true;
		submitDesc.useMaterialCulling = true;
		submitDesc.alphaTexturedAdditive = true;
		DrawMesh(*mesh, submitDesc, std::numeric_limits<uint8_t>::max());
	}
}
