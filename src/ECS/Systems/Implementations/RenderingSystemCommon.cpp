/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
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
#include "ECS/Components/Hand.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/MorphWithTerrain.h"
#include "ECS/Components/Stream.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Graphics/DebugLines.h"
#include "Graphics/GraphicsHandleBgfx.h"
#include "Graphics/ShaderManager.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;

RenderContext::RenderContext()
    : instanceUniformBuffer(BGFX_INVALID_HANDLE)
    , treeInstanceUniformBuffer(BGFX_INVALID_HANDLE)
    , partialBuildInstanceBuffer(BGFX_INVALID_HANDLE)
{
}
RenderContext::~RenderContext()
{
	for (const auto& handle : {instanceUniformBuffer, treeInstanceUniformBuffer, partialBuildInstanceBuffer})
	{
		if (bgfx::isValid(toBgfx(handle)))
		{
			bgfx::destroy(toBgfx(handle));
			bgfx::frame();
			bgfx::frame();
		}
	}
}

RenderingSystemCommon::~RenderingSystemCommon() = default;

void RenderingSystemCommon::SetDirty()
{
	_renderContext.dirty = true;
}

void RenderingSystemCommon::SetLayoutDirty()
{
	_renderContext.dirty = true;
	_renderContext.layoutDirty = true;
}

void RenderingSystemCommon::PrepareDraw(bool drawBoundingBox, bool drawFootpaths, bool drawStreams)
{
	auto& registry = Locator::entitiesRegistry::value();

	_renderContext.animatedBoneMatrices.clear();
	_renderContext.handMirrored = false;
	registry.Each<const Hand, const Mesh, const Transform>(
	    [this](const Hand& hand, const Mesh& mesh, const Transform& transform) {
		    if (!hand.boneMatrices.empty())
		    {
			    _renderContext.animatedBoneMatrices.insert_or_assign(mesh.id, hand.boneMatrices);
			    _renderContext.handMirrored = transform.scale.x * transform.scale.y * transform.scale.z < 0.0f;
		    }
	    });

	_renderContext.streamSegments.clear();
	registry.Each<const StreamSegment, const Transform>(
	    // The segment tag is empty, so only the transform is passed
	    [this](const Transform& transform) {
		    _renderContext.streamSegments.push_back(glm::translate(transform.position) * glm::mat4(transform.rotation) *
		                                            glm::scale(transform.scale));
	    });

	const bool optionsChanged = _renderContext.hasBoundingBoxes != drawBoundingBox ||
	                            (_renderContext.footpaths != nullptr) != drawFootpaths ||
	                            (_renderContext.streams != nullptr) != drawStreams;
	// While things only move, the draw lists stay as they are and only the instances are uploaded again
	if (!_renderContext.layoutDirty && !optionsChanged)
	{
		if (_renderContext.dirty && !UploadUniformsKeepingDescs(drawBoundingBox))
		{
			_renderContext.layoutDirty = true;
		}
		else
		{
			_renderContext.dirty = false;
		}
	}

	if (_renderContext.layoutDirty || optionsChanged)
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
			std::vector<graphics::DebugLines::Vertex> edges;
			registry.Each<const Stream>([&edges](const Stream& ent) {
				const auto color = glm::vec4(1, 0, 0, 1);
				for (size_t i = 1; i < ent.points.size(); ++i)
				{
					edges.push_back({glm::vec4(ent.points[i - 1], 1.0f), color});
					edges.push_back({glm::vec4(ent.points[i], 1.0f), color});
				}
			});

			if (!edges.empty())
			{
				_renderContext.streams =
				    graphics::DebugLines::CreateDebugLines(edges.data(), static_cast<uint32_t>(edges.size()));
			}
		}

		_renderContext.dirty = false;
		_renderContext.layoutDirty = false;
		_renderContext.hasBoundingBoxes = drawBoundingBox;
	}
}
