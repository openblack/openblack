/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Registry.h"

#include "3D/Camera.h"
#include "3D/L3DMesh.h"
#include "3D/MeshLookup.h"
#include "3D/MeshPack.h"
#include "AllMeshes.h"
#include "Entities/Components/Abode.h"
#include "Entities/Components/AnimatedStatic.h"
#include "Entities/Components/Stream.h"
#include "Entities/Components/Transform.h"
#include "Entities/Components/Tree.h"
#include "Game.h"
#include "Graphics/DebugLines.h"
#include "Graphics/ShaderManager.h"

#include <Entities/Components/Hand.h>
#include <algorithm>

namespace openblack::entities
{

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
	_registry.view<const Tree, const Transform>().each(
	    [&meshIds, &instanceCount](const Tree& entity, const Transform& transform) {
		    const auto meshId = treeMeshLookup[entity.treeInfo];
		    auto count = meshIds.insert(std::make_pair(meshId, 0));
		    count.first->second++;
		    instanceCount++;
	    });

	// Abodes
	_registry.view<const Abode, const Transform>().each(
	    [&meshIds, &instanceCount](const Abode& entity, const Transform& transform) {
		    const auto meshId = abodeMeshLookup[entity.abodeInfo];
		    auto count = meshIds.insert(std::make_pair(meshId, 0));
		    count.first->second++;
		    instanceCount++;
	    });

	// Villagers
	_registry.view<const Villager, const Transform>().each(
	    [&meshIds, &instanceCount](const Villager& villager, const Transform& transform) {
		    const auto villagerType = villager.GetVillagerType();
		    const auto meshId = villagerMeshLookup[villagerType];
		    auto count = meshIds.insert(std::make_pair(meshId, 0));
		    count.first->second++;
		    instanceCount++;
	    });

	// Animated Statics
	_registry.view<const AnimatedStatic, const Transform>().each(
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
	_registry.view<const MobileStatic, const Transform>().each(
	    [&meshIds, &instanceCount](const MobileStatic& entity, const Transform& transform) {
		    const auto meshId = mobileStaticMeshLookup[entity.type];
		    auto count = meshIds.insert(std::make_pair(meshId, 0));
		    count.first->second++;
		    instanceCount++;
	    });

	// Features
	_registry.view<const Feature, const Transform>().each(
	    [&meshIds, &instanceCount](const Feature& entity, const Transform& transform) {
		    const auto meshId = featureMeshLookup[entity.type];
		    auto count = meshIds.insert(std::make_pair(meshId, 0));
		    count.first->second++;
		    instanceCount++;
	    });

	// Fields
	_registry.view<const Field, const Transform>().each(
	    [&meshIds, &instanceCount](const Field& entity, const Transform& transform) {
		    const auto meshId = MeshPackId::TreeWheat;
		    auto count = meshIds.insert(std::make_pair(static_cast<MeshId>(meshId), 0));
		    count.first->second++;
		    instanceCount++;
	    });

	// Forests
	_registry.view<const Forest, const Transform>().each(
	    [&meshIds, &instanceCount](const Forest& entity, const Transform& transform) {
		    const auto meshId = MeshPackId::FeatureForest;
		    auto count = meshIds.insert(std::make_pair(static_cast<MeshId>(meshId), 0));
		    count.first->second++;
		    instanceCount++;
	    });

	// Mobile Objects
	_registry.view<const MobileObject, const Transform>().each(
	    [&meshIds, &instanceCount](const MobileObject& entity, const Transform& transform) {
		    const auto meshId = mobileObjectMeshLookup[entity.type];
		    auto count = meshIds.insert(std::make_pair(meshId, 0));
		    count.first->second++;
		    instanceCount++;
	    });

	// Hands
	_registry.view<const Hand, const Transform>().each(
	    [&meshIds, &instanceCount](const Hand& entity, const Transform& transform) {
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
	_registry.view<const Tree, const Transform>().each(
	    [&renderCtx, &uniformOffsets, prepareDrawBoundingBox](const Tree& entity, const Transform& transform) {
		    const auto meshId = treeMeshLookup[entity.treeInfo];
		    auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		    auto desc = renderCtx.instancedDrawDescs.find(meshId);
		    renderCtx.instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		    prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, -1);
		    offset.first->second++;
	    });

	// Abodes
	_registry.view<const Abode, const Transform>().each(
	    [&renderCtx, &uniformOffsets, prepareDrawBoundingBox](const Abode& entity, const Transform& transform) {
		    const auto meshId = abodeMeshLookup[entity.abodeInfo];
		    auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		    auto desc = renderCtx.instancedDrawDescs.find(meshId);
		    renderCtx.instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		    prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 0);
		    offset.first->second++;
	    });

	// Villagers
	_registry.view<const Villager, const Transform>().each(
	    [this, &renderCtx, &uniformOffsets, prepareDrawBoundingBox](const Villager& villager, const Transform& transform) {
		    const auto villagerType = villager.GetVillagerType();
		    const auto meshId = villagerMeshLookup[villagerType];
		    auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		    auto desc = renderCtx.instancedDrawDescs.find(meshId);
		    renderCtx.instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		    prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 0);
		    offset.first->second++;
	    });

	// Animated Statics
	_registry.view<const AnimatedStatic, const Transform>().each(
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
	_registry.view<const MobileStatic, const Transform>().each(
	    [&renderCtx, &uniformOffsets, prepareDrawBoundingBox](const MobileStatic& entity, const Transform& transform) {
		    const auto meshId = mobileStaticMeshLookup[entity.type];
		    auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		    auto desc = renderCtx.instancedDrawDescs.find(meshId);
		    renderCtx.instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		    prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 1);
		    offset.first->second++;
	    });

	// Features
	_registry.view<const Feature, const Transform>().each(
	    [&renderCtx, &uniformOffsets, prepareDrawBoundingBox](const Feature& entity, const Transform& transform) {
		    const auto meshId = featureMeshLookup[entity.type];
		    auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		    auto desc = renderCtx.instancedDrawDescs.find(meshId);
		    renderCtx.instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		    prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 1);
		    offset.first->second++;
	    });

	// Fields
	_registry.view<const Field, const Transform>().each(
	    [&renderCtx, &uniformOffsets, prepareDrawBoundingBox](const Field& entity, const Transform& transform) {
		    const MeshId meshId = static_cast<MeshId>(MeshPackId::TreeWheat);
		    auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		    auto desc = renderCtx.instancedDrawDescs.find(meshId);
		    renderCtx.instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		    prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 0);
		    offset.first->second++;
	    });

	// Forests
	_registry.view<const Forest, const Transform>().each(
	    [&renderCtx, &uniformOffsets, prepareDrawBoundingBox](const Forest& entity, const Transform& transform) {
		    const MeshId meshId = static_cast<MeshId>(MeshPackId::FeatureForest);
		    auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		    auto desc = renderCtx.instancedDrawDescs.find(meshId);
		    renderCtx.instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		    prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 0);
		    offset.first->second++;
	    });

	// Mobile Objects
	_registry.view<const MobileObject, const Transform>().each(
	    [&renderCtx, &uniformOffsets, prepareDrawBoundingBox](const MobileObject& entity, const Transform& transform) {
		    const auto meshId = mobileObjectMeshLookup[entity.type];
		    auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		    auto desc = renderCtx.instancedDrawDescs.find(meshId);
		    renderCtx.instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		    prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 1);
		    offset.first->second++;
	    });

	// Hands
	_registry.view<const Hand, const Transform>().each(
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

void Registry::PrepareDraw(bool drawBoundingBox, bool drawBoundingStreams)
{
	auto& renderCtx = Context().renderContext;

	if (renderCtx.dirty || renderCtx.hasBoundingBoxes != drawBoundingBox ||
	    (renderCtx.streams != nullptr) != drawBoundingStreams)
	{
		PrepareDrawDescs(drawBoundingBox);
		PrepareDrawUploadUniforms(drawBoundingBox);

		renderCtx.boundingBox.reset();
		if (drawBoundingBox)
		{
			renderCtx.boundingBox = graphics::DebugLines::CreateBox(glm::vec4(1.0f, 0.0f, 0.0f, 0.5f));
		}

		renderCtx.streams.reset();
		if (drawBoundingStreams)
		{
			uint32_t streamEdgeCount = 0;
			_registry.view<const Stream>().each([&streamEdgeCount](const Stream& com) {
				for (const auto& from : com.streamNodes)
				{
					streamEdgeCount += from.edges.size();
				}
			});
			std::vector<graphics::DebugLines::Vertex> streamEdges;
			streamEdges.reserve(streamEdgeCount * 2);
			_registry.view<const Stream>().each([&streamEdges](const Stream& com) {
				const auto color = glm::vec4(1, 0, 0, 1);
				for (const auto& from : com.streamNodes)
				{
					for (const auto& to : from.edges)
					{
						streamEdges.push_back({glm::vec4(from.position, 1.0f), color});
						streamEdges.push_back({glm::vec4(to.position, 1.0f), color});
					}
				}
			});

			if (!streamEdges.empty())
			{
				renderCtx.streams = graphics::DebugLines::CreateDebugLines(streamEdges.data(), streamEdges.size());
			}
		}

		renderCtx.dirty = false;
		renderCtx.hasBoundingBoxes = drawBoundingBox;
	}
}
} // namespace openblack::entities
