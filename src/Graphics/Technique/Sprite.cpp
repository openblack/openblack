/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ECS/Components/Sprite.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include "ECS/Components/SpriteMesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Graphics/Mesh.h"
#include "Graphics/Primitive.h"
#include "Graphics/ShaderManagerInterface.h"
#include "Graphics/VertexBuffer.h"
#include "Locator.h"
#include "TechniqueCommon.h"
#include "TechniqueManager.h"

using namespace openblack::graphics;

template <>
void openblack::graphics::renderTechnique<Technique::Sprites>(RendererInterface::DrawSceneDesc desc)
{
	const auto* spriteShader = Locator::shaderManager::value().GetShader("Sprite");
	auto& registry = Locator::entitiesRegistry::value();
	using namespace ecs::components;

	auto entity = registry.Front<SpriteMesh>();
	const auto& spritePlane = registry.Get<const SpriteMesh>(entity);

	registry.Each<const Sprite, const Transform>(
	    [&spritePlane, &spriteShader, &desc](const Sprite& sprite, const Transform& transform) {
		    auto modelMatrix = glm::mat4(1.0f);
		    modelMatrix = glm::translate(modelMatrix, transform.position);
		    modelMatrix *= glm::mat4(transform.rotation);
		    modelMatrix = glm::scale(modelMatrix, transform.scale);

		    glm::vec4 u_sampleRect(sprite.uvExtent, sprite.uvMin);

		    bgfx::setTransform(glm::value_ptr(modelMatrix));
		    spriteShader->SetUniformValue("u_sampleRect", glm::value_ptr(u_sampleRect));
		    spriteShader->SetUniformValue("u_tint", glm::value_ptr(sprite.tint));
		    spriteShader->SetTextureSampler("s_diffuse", 0, sprite.texture);

		    spritePlane.plane->GetVertexBuffer().Bind();

		    bgfx::setState(0 | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
		                   BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE) |
		                   BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_ADD));

		    bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), spriteShader->GetRawHandle());
	    });
}
