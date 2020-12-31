/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Registry.h"

#include <algorithm>

#include "3D/Camera.h"
#include "3D/L3DMesh.h"
#include "3D/MeshPack.h"
#include "AllMeshes.h"
#include "Common/MeshLookup.h"
#include "Entities/Components/Abode.h"
#include "Entities/Components/AnimatedStatic.h"
#include "Entities/Components/Field.h"
#include "Entities/Components/Forest.h"
#include "Entities/Components/Mesh.h"
#include "Entities/Components/Stream.h"
#include "Entities/Components/Transform.h"
#include "Entities/Components/Tree.h"
#include "Game.h"
#include "Graphics/DebugLines.h"
#include "Graphics/ShaderManager.h"

#include <Entities/Components/Footpath.h>
#include <Entities/Components/Hand.h>

namespace openblack::entities
{

using namespace components;

Registry::Registry()
{
	_registry.set<RegistryContext>();
}

RegistryContext& Registry::Context()
{
	return _registry.ctx<RegistryContext>();
}

const RegistryContext& Registry::Context() const
{
	return _registry.ctx<RegistryContext>();
}

void Registry::PrepareDrawDescs(bool drawBoundingBox)
{
	// Count number of instances
	uint32_t instanceCount = 0;
	std::map<MeshId, uint32_t> meshIds;

	Each<const Mesh, const Transform>([&meshIds, &instanceCount](const Mesh& mesh, const Transform&) {
		auto count = meshIds.insert(std::make_pair(mesh.id, mesh.submeshId));
		count.first->second++;
		instanceCount++;
	});

	if (drawBoundingBox)
	{
		instanceCount *= 2;
	}

	auto& renderCtx = Context().renderContext;

	// Recreate instancing uniform buffer if it is too small
	if (renderCtx.instanceUniforms.size() < instanceCount)
	{
		if (bgfx::isValid(renderCtx.instanceUniformBuffer))
		{
			bgfx::destroy(renderCtx.instanceUniformBuffer);
		}
		bgfx::VertexLayout layout;
		layout.begin()
		    .add(bgfx::Attrib::TexCoord7, 4, bgfx::AttribType::Float)
		    .add(bgfx::Attrib::TexCoord6, 4, bgfx::AttribType::Float)
		    .add(bgfx::Attrib::TexCoord5, 4, bgfx::AttribType::Float)
		    .add(bgfx::Attrib::TexCoord4, 4, bgfx::AttribType::Float)
		    .end();
		renderCtx.instanceUniformBuffer = bgfx::createDynamicVertexBuffer(instanceCount, layout);
		renderCtx.instanceUniforms.resize(instanceCount);
	}

	// Determine uniform buffer offsets and instance count for draw
	uint32_t offset = 0;
	renderCtx.instancedDrawDescs.clear();
	for (const auto& [meshId, count] : meshIds)
	{
		renderCtx.instancedDrawDescs.emplace(std::piecewise_construct, std::forward_as_tuple(meshId),
		                                     std::forward_as_tuple(offset, count));
		offset += count;
	}
}

void Registry::PrepareDrawUploadUniforms(bool drawBoundingBox)
{
	auto& renderCtx = Context().renderContext;

	// Store offsets of uniforms for descs
	std::map<MeshId, uint32_t> uniformOffsets;

	// Set transforms for instanced draw at offsets
	Each<const Mesh, const Transform>(
	    [&renderCtx, &uniformOffsets, drawBoundingBox](const Mesh& mesh, const Transform& transform) {
		    auto offset = uniformOffsets.insert(std::make_pair(mesh.id, 0));
		    auto desc = renderCtx.instancedDrawDescs.find(mesh.id);
		    renderCtx.instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		    uint32_t idx = desc->second.offset + offset.first->second;
		    if (drawBoundingBox)
		    {
			    const L3DMesh& l3dMesh = Game::instance()->GetMeshPack().GetMesh(mesh.id);
			    auto submeshId = (l3dMesh.GetNumSubMeshes() + mesh.bbSubmeshId) % l3dMesh.GetNumSubMeshes();
			    auto box = l3dMesh.GetSubMeshes()[submeshId]->GetBoundingBox();
			    auto boxMatrix = static_cast<glm::mat4>(transform) * glm::translate(box.center()) * glm::scale(box.size());
			    renderCtx.instanceUniforms[idx + renderCtx.instanceUniforms.size() / 2] = boxMatrix;
		    }
		    offset.first->second++;
	    });

	if (!renderCtx.instanceUniforms.empty())
	{
		bgfx::update(renderCtx.instanceUniformBuffer, 0,
		             bgfx::makeRef(renderCtx.instanceUniforms.data(), renderCtx.instanceUniforms.size() * sizeof(glm::mat4)));
	}
}

void Registry::PrepareDraw(bool drawBoundingBox, bool drawFootpaths, bool drawStreams)
{
	auto& renderCtx = Context().renderContext;

	if (renderCtx.dirty || renderCtx.hasBoundingBoxes != drawBoundingBox || (renderCtx.footpaths != nullptr) != drawFootpaths ||
	    (renderCtx.streams != nullptr) != drawStreams)
	{
		PrepareDrawDescs(drawBoundingBox);
		PrepareDrawUploadUniforms(drawBoundingBox);

		renderCtx.boundingBox.reset();
		if (drawBoundingBox)
		{
			renderCtx.boundingBox = graphics::DebugLines::CreateBox(glm::vec4(1.0f, 0.0f, 0.0f, 0.5f));
		}

		renderCtx.footpaths.reset();
		if (drawFootpaths)
		{
			uint32_t nodeCount = 0;
			Each<const Footpath>(
			    [&nodeCount](const Footpath& ent) { nodeCount += 2 * std::max(static_cast<int>(ent.nodes.size()) - 1, 0); });

			std::vector<graphics::DebugLines::Vertex> edges;
			edges.reserve(nodeCount);
			Each<const Footpath>([&edges](const Footpath& ent) {
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
				renderCtx.footpaths = graphics::DebugLines::CreateDebugLines(edges.data(), edges.size());
			}
		}

		renderCtx.streams.reset();
		if (drawStreams)
		{
			uint32_t edgeCount = 0;
			Each<const Stream>([&edgeCount](const Stream& ent) {
				for (const auto& from : ent.nodes)
				{
					edgeCount += from.edges.size();
				}
			});
			std::vector<graphics::DebugLines::Vertex> edges;
			edges.reserve(edgeCount * 2);
			Each<const Stream>([&edges](const Stream& ent) {
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
				renderCtx.streams = graphics::DebugLines::CreateDebugLines(edges.data(), edges.size());
			}
		}

		renderCtx.dirty = false;
		renderCtx.hasBoundingBoxes = drawBoundingBox;
	}
}

void Registry::SetDirty()
{
	auto& renderCtx = Context().renderContext;
	renderCtx.dirty = false;
}
} // namespace openblack::entities
