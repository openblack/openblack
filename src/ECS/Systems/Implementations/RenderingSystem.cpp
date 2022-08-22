/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "RenderingSystem.h"

#include <glm/gtx/transform.hpp>

#include "3D/L3DMesh.h"
#include "ECS/Components/CameraPathNode.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/MorphWithTerrain.h"
#include "ECS/Components/Stream.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Game.h"
#include "Graphics/DebugLines.h"
#include "Graphics/ShaderManager.h"
#include "Locator.h"
#include "Resources/Resources.h"

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

RenderingSystemInterface::~RenderingSystemInterface() = default;

RenderingSystem::~RenderingSystem() = default;

void RenderingSystem::PrepareDrawDescs(bool drawBoundingBox)
{
	auto& registry = Game::Instance()->GetEntityRegistry();

	// Count number of instances
	uint32_t instanceCount = 0;
	std::unordered_map<entt::id_type, std::pair<uint32_t, bool>> meshIds;

	auto prep = [&meshIds, &instanceCount](const Mesh& mesh, bool morphWithTerrain) {
		auto count = meshIds.insert(std::make_pair(mesh.id, std::make_pair(mesh.submeshId, morphWithTerrain)));
		count.first->second.first++;
		instanceCount++;
	};

	registry.Each<const Mesh, const Transform>([&prep](const Mesh& mesh, const Transform& /*unused*/) { prep(mesh, false); },
	                                           entt::exclude<MorphWithTerrain>);
	registry.Each<const Mesh, const Transform, const MorphWithTerrain>(
	    [&prep](const Mesh& mesh, const Transform& /*unused*/, const MorphWithTerrain& /*unused*/) { prep(mesh, true); });

	if (drawBoundingBox)
	{
		instanceCount *= 2;
	}

	// Recreate instancing uniform buffer if it is too small
	if (_renderContext.instanceUniforms.size() < instanceCount)
	{
		if (bgfx::isValid(_renderContext.instanceUniformBuffer))
		{
			bgfx::destroy(_renderContext.instanceUniformBuffer);
		}
		bgfx::VertexLayout layout;
		layout.begin()
		    .add(bgfx::Attrib::TexCoord7, 4, bgfx::AttribType::Float)
		    .add(bgfx::Attrib::TexCoord6, 4, bgfx::AttribType::Float)
		    .add(bgfx::Attrib::TexCoord5, 4, bgfx::AttribType::Float)
		    .add(bgfx::Attrib::TexCoord4, 4, bgfx::AttribType::Float)
		    .end();
		_renderContext.instanceUniformBuffer = bgfx::createDynamicVertexBuffer(instanceCount, layout);
		_renderContext.instanceUniforms.resize(instanceCount);
	}

	// Determine uniform buffer offsets and instance count for draw
	uint32_t offset = 0;
	_renderContext.instancedDrawDescs.clear();
	for (const auto& [meshId, desc] : meshIds)
	{
		_renderContext.instancedDrawDescs.emplace(std::piecewise_construct, std::forward_as_tuple(meshId),
		                                          std::forward_as_tuple(offset, desc.first, desc.second));
		offset += desc.first;
	}
}

void RenderingSystem::PrepareDrawUploadUniforms(bool drawBoundingBox)
{
	auto& registry = Game::Instance()->GetEntityRegistry();

	// Store offsets of uniforms for descs
	std::map<entt::id_type, uint32_t> uniformOffsets;

	// Set transforms for instanced draw at offsets
	registry.Each<const Mesh, const Transform>(
	    [this, &uniformOffsets, drawBoundingBox](const Mesh& mesh, const Transform& transform) {
		    auto offset = uniformOffsets.insert(std::make_pair(mesh.id, 0));
		    auto desc = _renderContext.instancedDrawDescs.find(mesh.id);

		    glm::mat4 modelMatrix = glm::mat4(1.0f);
		    modelMatrix = glm::translate(modelMatrix, transform.position);
		    modelMatrix *= glm::mat4(transform.rotation);
		    modelMatrix = glm::scale(modelMatrix, transform.scale);

		    uint32_t idx = desc->second.offset + offset.first->second;
		    _renderContext.instanceUniforms[idx] = modelMatrix;
		    if (drawBoundingBox)
		    {
			    auto l3dMesh = entt::locator<resources::ResourcesInterface>::value().GetMeshes().Handle(mesh.id);
			    auto box = l3dMesh->GetBoundingBox();
			    auto boxMatrix = modelMatrix * glm::translate(box.Center()) * glm::scale(box.Size());
			    _renderContext.instanceUniforms[idx + _renderContext.instanceUniforms.size() / 2] = boxMatrix;
		    }
		    offset.first->second++;
	    });

	if (!_renderContext.instanceUniforms.empty())
	{
		const auto size = static_cast<uint32_t>(_renderContext.instanceUniforms.size() * sizeof(glm::mat4));
		bgfx::update(_renderContext.instanceUniformBuffer, 0, bgfx::makeRef(_renderContext.instanceUniforms.data(), size));
	}
}

void RenderingSystem::SetDirty()
{
	_renderContext.dirty = true;
}

void RenderingSystem::PrepareDraw(bool drawBoundingBox, bool drawFootpaths, bool drawStreams)
{
	auto& registry = Game::Instance()->GetEntityRegistry();

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
