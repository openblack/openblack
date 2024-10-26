/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ECS/Components/DebugCross.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

#include "ECS/Registry.h"
#include "Graphics/ShaderManagerInterface.h"
#include "Graphics/VertexBuffer.h"
#include "Locator.h"
#include "TechniqueCommon.h"
#include "TechniqueManager.h"

using namespace openblack::graphics;

template <>
void openblack::graphics::renderTechnique<Technique::DebugCross>(RendererInterface::DrawSceneDesc desc)
{
	const auto* debugShader = Locator::shaderManager::value().GetShader("DebugLine");

	auto& registry = Locator::entitiesRegistry::value();

	registry.Each<const ecs::components::DebugCross>([&desc, &debugShader](const ecs::components::DebugCross& ent) {
		bgfx::setTransform(glm::value_ptr(ent.debugCrossPose));
		ent.debugCross->GetVertexBuffer().Bind();
		bgfx::setState(k_BgfxDefaultStateInvertedZ | BGFX_STATE_PT_LINES);
		bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), debugShader->GetRawHandle());
	});
}
