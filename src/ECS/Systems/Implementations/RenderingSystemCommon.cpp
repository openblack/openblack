/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "RenderingSystemCommon.h"

#include <glm/gtx/transform.hpp>

#include "3D/L3DMesh.h"
#include "ECS/Components/CameraPathNode.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/MorphWithTerrain.h"
#include "ECS/Components/Stream.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Graphics/DebugLines.h"
#include "Graphics/ShaderManager.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;

RenderContext::RenderContext()
    : instanceUniformBuffer(BGFX_INVALID_HANDLE)
{
}
RenderContext::~RenderContext()
{
	if (bgfx::isValid(instanceUniformBuffer))
	{
		bgfx::destroy(instanceUniformBuffer);
		bgfx::frame();
		bgfx::frame();
	}
}

RenderingSystemCommon::~RenderingSystemCommon() = default;

void RenderingSystemCommon::SetDirty()
{
	_renderContext.dirty = true;
}

void RenderingSystemCommon::PrepareDraw(bool drawBoundingBox, bool drawFootpaths, bool drawStreams)
{
	auto& registry = Locator::entitiesRegistry::value();

	if (_renderContext.dirty || _renderContext.hasBoundingBoxes != drawBoundingBox ||
	    (_renderContext.footpaths != nullptr) != drawFootpaths || (_renderContext.streams != nullptr) != drawStreams)
	{
		PrepareDrawDescs(drawBoundingBox);
		PrepareDrawUploadUniforms(drawBoundingBox);

		_renderContext.boundingBox.reset();
		if (drawBoundingBox)
		{
			_renderContext.boundingBox = graphics::DebugLines::CreateBox(glm::vec4(1.0f, 0.0f, 0.0f, 0.5f));
		}

		_renderContext.footpaths.reset();
		if (drawFootpaths)
		{
			uint32_t nodeCount = 0;
			registry.Each<const Footpath>(
			    [&nodeCount](const Footpath& ent) { nodeCount += 2 * std::max(static_cast<int>(ent.nodes.size()) - 1, 0); });

			std::vector<graphics::DebugLines::Vertex> edges;
			edges.reserve(nodeCount);
			registry.Each<const Footpath>([&edges](const Footpath& ent) {
				const auto color = glm::vec4(0, 1, 0, 1);
				const auto offset = glm::vec3(0, 1, 0);
				for (int i = 0; i < static_cast<int>(ent.nodes.size()) - 1; ++i)
				{
					edges.push_back({glm::vec4(ent.nodes[i].position + offset, 1.0f), color});
					edges.push_back({glm::vec4(ent.nodes[i + 1].position + offset, 1.0f), color});
				}
			});
			if (!edges.empty())
			{
				_renderContext.footpaths =
				    graphics::DebugLines::CreateDebugLines(edges.data(), static_cast<uint32_t>(edges.size()));
			}
		}

		_renderContext.streams.reset();
		if (drawStreams)
		{
			uint32_t edgeCount = 0;
			registry.Each<const Stream>([&edgeCount](const Stream& ent) {
				for (const auto& from : ent.nodes)
				{
					edgeCount += static_cast<uint32_t>(from.edges.size());
				}
			});
			std::vector<graphics::DebugLines::Vertex> edges;
			edges.reserve(edgeCount * 2);
			registry.Each<const Stream>([&edges](const Stream& ent) {
				const auto color = glm::vec4(1, 0, 0, 1);
				for (const auto& from : ent.nodes)
				{
					for (const auto& to : from.edges)
					{
						edges.push_back({glm::vec4(from.position, 1.0f), color});
						edges.push_back({glm::vec4(to.position, 1.0f), color});
					}
				}
			});

			if (!edges.empty())
			{
				_renderContext.streams =
				    graphics::DebugLines::CreateDebugLines(edges.data(), static_cast<uint32_t>(edges.size()));
			}
		}

		// Camera paths
		{
			uint32_t edgeCount = 0;
			registry.Each<const CameraPathNode, const Transform>(
			    [&edgeCount](const CameraPathNode& vertex, [[maybe_unused]] const Transform& transform) {
				    if (vertex.next != entt::null)
				    {
					    edgeCount += 2;
				    }
			    });
			std::vector<graphics::DebugLines::Vertex> edges;
			edges.reserve(edgeCount * 2);
			registry.Each<const CameraPathNode, const Transform>(
			    [&edges, &registry](const CameraPathNode& vertex, const Transform& transform) {
				    const auto color = glm::vec4(1, 0, 0, 1);
				    // The last node will point to null
				    if (vertex.next != entt::null)
				    {
					    const auto& destination = registry.Get<Transform>(vertex.next);
					    edges.push_back({glm::vec4(transform.position, 1.0f), color});
					    edges.push_back({glm::vec4(destination.position, 1.0f), color});
				    }
			    });

			if (!edges.empty())
			{
				_renderContext.streams =
				    graphics::DebugLines::CreateDebugLines(edges.data(), static_cast<uint32_t>(edges.size()));
			}
		}

		_renderContext.dirty = false;
		_renderContext.hasBoundingBoxes = drawBoundingBox;
	}
}
