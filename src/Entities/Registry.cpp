/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
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

	// Trees
	Each<const Tree, const Transform>([&meshIds, &instanceCount](const Tree& entity, const Transform& transform) {
		const auto meshId = treeMeshLookup[entity.type];
		auto count = meshIds.insert(std::make_pair(meshId, 0));
		count.first->second++;
		instanceCount++;
	});

	// Abodes
	Each<const Abode, const Transform>([&meshIds, &instanceCount](const Abode& entity, const Transform& transform) {
		const auto meshId = abodeMeshLookup[entity.abodeInfo];
		auto count = meshIds.insert(std::make_pair(meshId, 0));
		count.first->second++;
		instanceCount++;
	});

	// Villagers
	Each<const Villager, const Transform>([&meshIds, &instanceCount](const Villager& villager, const Transform& transform) {
		const auto villagerType = villager.GetVillagerType();
		const auto meshId = villagerMeshLookup[villagerType];
		auto count = meshIds.insert(std::make_pair(meshId, 0));
		count.first->second++;
		instanceCount++;
	});

	// Animated Statics
	Each<const AnimatedStatic, const Transform>(
	    [&meshIds, &instanceCount](const AnimatedStatic& entity, const Transform& transform) {
		    // temporary-ish:
		    MeshPackId meshId = MeshPackId::Dummy;
		    if (entity.type == "Norse Gate")
		    {
			    meshId = MeshPackId::BuildingNorseGate;
		    }
		    else if (entity.type == "Gate Stone Plinth")
		    {
			    meshId = MeshPackId::ObjectGateTotemPlinthe;
		    }
		    else if (entity.type == "Piper Cave Entrance")
		    {
			    meshId = MeshPackId::BuildingMineEntrance;
		    }
		    auto count = meshIds.insert(std::make_pair(static_cast<MeshId>(meshId), 0));
		    count.first->second++;
		    instanceCount++;
	    });

	// Mobile Statics
	Each<const MobileStatic, const Transform>(
	    [&meshIds, &instanceCount](const MobileStatic& entity, const Transform& transform) {
		    const auto meshId = mobileStaticMeshLookup[entity.type];
		    auto count = meshIds.insert(std::make_pair(meshId, 0));
		    count.first->second++;
		    instanceCount++;
	    });

	// Features
	Each<const Feature, const Transform>([&meshIds, &instanceCount](const Feature& entity, const Transform& transform) {
		const auto meshId = featureMeshLookup[entity.type];
		auto count = meshIds.insert(std::make_pair(meshId, 0));
		count.first->second++;
		instanceCount++;
	});

	// Fields
	Each<const Field, const Transform>([&meshIds, &instanceCount](const Field& entity, const Transform& transform) {
		const auto meshId = MeshPackId::TreeWheat;
		auto count = meshIds.insert(std::make_pair(static_cast<MeshId>(meshId), 0));
		count.first->second++;
		instanceCount++;
	});

	// Forests
	Each<const Forest, const Transform>([&meshIds, &instanceCount](const Forest& entity, const Transform& transform) {
		const auto meshId = MeshPackId::FeatureForest;
		auto count = meshIds.insert(std::make_pair(static_cast<MeshId>(meshId), 0));
		count.first->second++;
		instanceCount++;
	});

	// Mobile Objects
	Each<const MobileObject, const Transform>(
	    [&meshIds, &instanceCount](const MobileObject& entity, const Transform& transform) {
		    const auto meshId = mobileObjectMeshLookup[entity.type];
		    auto count = meshIds.insert(std::make_pair(meshId, 0));
		    count.first->second++;
		    instanceCount++;
	    });

	// Hands
	Each<const Hand, const Transform>([&meshIds, &instanceCount](const Hand& entity, const Transform& transform) {
		const auto meshId = MeshId(999);
		auto count = meshIds.insert(std::make_pair(meshId, 0));
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

	// Set transforms for instanced draw at offsets
	auto prepareDrawBoundingBox = [&renderCtx, drawBoundingBox](uint32_t idx, const Transform& transform, MeshId meshId,
	                                                            int8_t submeshId) {
		if (drawBoundingBox)
		{
			const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(meshId);
			auto box = mesh.GetSubMeshes()[(mesh.GetNumSubMeshes() + submeshId) % mesh.GetNumSubMeshes()]->GetBoundingBox();

			glm::mat4 boxMatrix = glm::mat4(1.0f);
			boxMatrix = glm::translate(boxMatrix, transform.position + box.center());
			//			boxMatrix           = glm::rotate(boxMatrix,
			// transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)); 			boxMatrix
			// = glm::rotate(boxMatrix, transform.rotation.y,
			// glm::vec3(0.0f, 1.0f, 0.0f)); 			boxMatrix           =
			// glm::rotate(boxMatrix, transform.rotation.z, glm::vec3(0.0f,
			// 0.0f, 1.0f));
			boxMatrix = glm::scale(boxMatrix, transform.scale * box.size());

			renderCtx.instanceUniforms[idx + renderCtx.instanceUniforms.size() / 2] = boxMatrix;
		}
	};
	// Store offsets of uniforms for descs
	std::map<MeshId, uint32_t> uniformOffsets;

	// Trees
	Each<const Tree, const Transform>(
	    [&renderCtx, &uniformOffsets, prepareDrawBoundingBox](const Tree& entity, const Transform& transform) {
		    const auto meshId = treeMeshLookup[entity.type];
		    auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		    auto desc = renderCtx.instancedDrawDescs.find(meshId);
		    renderCtx.instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		    prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, -1);
		    offset.first->second++;
	    });

	// Abodes
	Each<const Abode, const Transform>(
	    [&renderCtx, &uniformOffsets, prepareDrawBoundingBox](const Abode& entity, const Transform& transform) {
		    const auto meshId = abodeMeshLookup[entity.abodeInfo];
		    auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		    auto desc = renderCtx.instancedDrawDescs.find(meshId);
		    renderCtx.instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		    prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 0);
		    offset.first->second++;
	    });

	// Villagers
	Each<const Villager, const Transform>(
	    [&renderCtx, &uniformOffsets, prepareDrawBoundingBox](const Villager& villager, const Transform& transform) {
		    const auto villagerType = villager.GetVillagerType();
		    const auto meshId = villagerMeshLookup[villagerType];
		    auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		    auto desc = renderCtx.instancedDrawDescs.find(meshId);
		    renderCtx.instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		    prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 0);
		    offset.first->second++;
	    });

	// Animated Statics
	Each<const AnimatedStatic, const Transform>(
	    [&renderCtx, &uniformOffsets, prepareDrawBoundingBox](const AnimatedStatic& entity, const Transform& transform) {
		    // temporary-ish:
		    MeshPackId meshPackId = MeshPackId::Dummy;
		    if (entity.type == "Norse Gate")
		    {
			    meshPackId = MeshPackId::BuildingNorseGate;
		    }
		    else if (entity.type == "Gate Stone Plinth")
		    {
			    meshPackId = MeshPackId::ObjectGateTotemPlinthe;
		    }
		    else if (entity.type == "Piper Cave Entrance")
		    {
			    meshPackId = MeshPackId::BuildingMineEntrance;
		    }
		    auto meshId = static_cast<MeshId>(meshPackId);
		    auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		    auto desc = renderCtx.instancedDrawDescs.find(meshId);
		    renderCtx.instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		    prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 0);
		    offset.first->second++;
	    });

	// Mobile Statics
	Each<const MobileStatic, const Transform>(
	    [&renderCtx, &uniformOffsets, prepareDrawBoundingBox](const MobileStatic& entity, const Transform& transform) {
		    const auto meshId = mobileStaticMeshLookup[entity.type];
		    auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		    auto desc = renderCtx.instancedDrawDescs.find(meshId);
		    renderCtx.instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		    prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 1);
		    offset.first->second++;
	    });

	// Features
	Each<const Feature, const Transform>(
	    [&renderCtx, &uniformOffsets, prepareDrawBoundingBox](const Feature& entity, const Transform& transform) {
		    const auto meshId = featureMeshLookup[entity.type];
		    auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		    auto desc = renderCtx.instancedDrawDescs.find(meshId);
		    renderCtx.instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		    prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 1);
		    offset.first->second++;
	    });

	// Fields
	Each<const Field, const Transform>(
	    [&renderCtx, &uniformOffsets, prepareDrawBoundingBox](const Field& entity, const Transform& transform) {
		    const MeshId meshId = static_cast<MeshId>(MeshPackId::TreeWheat);
		    auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		    auto desc = renderCtx.instancedDrawDescs.find(meshId);
		    renderCtx.instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		    prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 0);
		    offset.first->second++;
	    });

	// Forests
	Each<const Forest, const Transform>(
	    [&renderCtx, &uniformOffsets, prepareDrawBoundingBox](const Forest& entity, const Transform& transform) {
		    const MeshId meshId = static_cast<MeshId>(MeshPackId::FeatureForest);
		    auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		    auto desc = renderCtx.instancedDrawDescs.find(meshId);
		    renderCtx.instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		    prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 0);
		    offset.first->second++;
	    });

	// Mobile Objects
	Each<const MobileObject, const Transform>(
	    [&renderCtx, &uniformOffsets, prepareDrawBoundingBox](const MobileObject& entity, const Transform& transform) {
		    const auto meshId = mobileObjectMeshLookup[entity.type];
		    auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		    auto desc = renderCtx.instancedDrawDescs.find(meshId);
		    renderCtx.instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		    prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 1);
		    offset.first->second++;
	    });

	// Hands
	Each<const Hand, const Transform>(
	    [&renderCtx, &uniformOffsets, prepareDrawBoundingBox](const Hand& entity, const Transform& transform) {
		    MeshId meshId = 999;
		    auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		    auto desc = renderCtx.instancedDrawDescs.find(meshId);
		    renderCtx.instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		    prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 1);
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
