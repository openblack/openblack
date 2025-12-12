/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "RenderingSystem.h"

#include <algorithm>
#include <iostream>

#include <glm/gtx/transform.hpp>

#include "3D/L3DMesh.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/MorphWithTerrain.h"
#include "ECS/Components/Stream.h"
#include "ECS/Components/Swayable.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Registry.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "Game.h"
#include "Graphics/DebugLines.h"
#include "Graphics/ShaderManager.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;

RenderingSystem::~RenderingSystem() = default;

void RenderingSystem::PrepareDrawDescs(bool drawBoundingBox)
{
	auto& registry = Locator::entitiesRegistry::value();

	// Count number of instances
	uint32_t instanceCount = 0;
	std::unordered_map<entt::id_type, std::pair<uint32_t, bool>> meshIds;

	auto prep = [&meshIds, &instanceCount](const Mesh& mesh, bool morphWithTerrain) {
		auto count = meshIds.insert(std::make_pair(mesh.id, std::make_pair(mesh.submeshId, morphWithTerrain)));
		count.first->second.first++;
		instanceCount++;
	};

	registry.Each<const Mesh, const Transform>([&prep](const Mesh& mesh, const Transform& /*unused*/) { prep(mesh, false); },
	                                           entt::exclude<MorphWithTerrain, Tree, TempleInteriorPart>);
	registry.Each<const Mesh, const Transform, const MorphWithTerrain>(
	    [&prep](const Mesh& mesh, const Transform& /*unused*/, const MorphWithTerrain& /*unused*/) { prep(mesh, true); },
	    entt::exclude<Tree>);

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

	// Prepare tree instances separately
	PrepareTreeDrawDescs(drawBoundingBox);
}

void RenderingSystem::PrepareTreeDrawDescs(bool drawBoundingBox)
{
	auto& registry = Locator::entitiesRegistry::value();

	// Count number of tree instances
	uint32_t treeInstanceCount = 0;
	std::unordered_map<entt::id_type, uint32_t> treeMeshIds;

	auto prepTree = [&treeMeshIds, &treeInstanceCount](const Mesh& mesh) {
		auto count = treeMeshIds.insert(std::make_pair(mesh.id, 1));
		if (!count.second)
		{
			count.first->second++;
		}
		treeInstanceCount++;
	};

	// Process Tree entities
	registry.Each<const Mesh, const Transform, const Tree>(
	    [&prepTree](const Mesh& mesh, const Transform& /*unused*/, const Tree& /*unused*/) { prepTree(mesh); });

	if (drawBoundingBox)
	{
		treeInstanceCount *= 2;
	}

	// Create tree instance buffer if needed
	if (_renderContext.treeInstanceData.size() < treeInstanceCount)
	{
		if (bgfx::isValid(_renderContext.treeInstanceUniformBuffer))
		{
			bgfx::destroy(_renderContext.treeInstanceUniformBuffer);
		}
		bgfx::VertexLayout layout;
		layout.begin()
		    .add(bgfx::Attrib::TexCoord7, 4, bgfx::AttribType::Float) // i_data0 (matrix row 0)
		    .add(bgfx::Attrib::TexCoord6, 4, bgfx::AttribType::Float) // i_data1 (matrix row 1)
		    .add(bgfx::Attrib::TexCoord5, 4, bgfx::AttribType::Float) // i_data2 (matrix row 2)
		    .add(bgfx::Attrib::TexCoord4, 4, bgfx::AttribType::Float) // i_data3 (matrix row 3)
		    .add(bgfx::Attrib::TexCoord3, 4, bgfx::AttribType::Float) // i_data4 (sway params)
		    .end();
		_renderContext.treeInstanceUniformBuffer = bgfx::createDynamicVertexBuffer(treeInstanceCount, layout);
		_renderContext.treeInstanceData.resize(treeInstanceCount);
	}

	// Determine tree uniform buffer offsets and instance count for draw
	uint32_t treeOffset = 0;
	_renderContext.treeInstancedDrawDescs.clear();
	for (const auto& [meshId, count] : treeMeshIds)
	{
		_renderContext.treeInstancedDrawDescs.emplace(std::piecewise_construct, std::forward_as_tuple(meshId),
		                                              std::forward_as_tuple(treeOffset, count, false));
		treeOffset += count;
	}
}

void RenderingSystem::PrepareDrawUploadUniforms(bool drawBoundingBox)
{
	auto& registry = Locator::entitiesRegistry::value();

	// Store offsets of uniforms for descs
	std::map<entt::id_type, uint32_t> uniformOffsets;

	// Set transforms for instanced draw at offsets
	registry.Each<const Mesh, const Transform>(
	    [this, &uniformOffsets, drawBoundingBox](const Mesh& mesh, const Transform& transform) {
		    auto offset = uniformOffsets.insert(std::make_pair(mesh.id, 0));
		    auto desc = _renderContext.instancedDrawDescs.find(mesh.id);

		    auto modelMatrix = glm::mat4(transform.rotation);
		    modelMatrix = glm::translate(modelMatrix, transform.position * transform.rotation);
		    modelMatrix = glm::scale(modelMatrix, transform.scale);

		    const uint32_t idx = desc->second.offset + offset.first->second;
		    _renderContext.instanceUniforms[idx] = modelMatrix;
		    if (drawBoundingBox)
		    {
			    auto l3dMesh = entt::locator<resources::ResourcesInterface>::value().GetMeshes().Handle(mesh.id);
			    auto box = l3dMesh->GetBoundingBox();
			    auto boxMatrix = modelMatrix * glm::translate(box.Center()) * glm::scale(box.Size());
			    _renderContext.instanceUniforms[idx + (_renderContext.instanceUniforms.size() / 2)] = boxMatrix;
		    }
		    offset.first->second++;
	    },
	    entt::exclude<TempleInteriorPart, Tree>);

	if (!_renderContext.instanceUniforms.empty())
	{
		const auto size = static_cast<uint32_t>(_renderContext.instanceUniforms.size() * sizeof(glm::mat4));
		bgfx::update(_renderContext.instanceUniformBuffer, 0, bgfx::makeRef(_renderContext.instanceUniforms.data(), size));
	}

	// Upload tree uniforms separately
	PrepareTreeDrawUploadUniforms(drawBoundingBox);
}

void RenderingSystem::PrepareTreeDrawUploadUniforms(bool drawBoundingBox)
{
	auto& registry = Locator::entitiesRegistry::value();

	// Store offsets of tree uniforms
	std::map<entt::id_type, uint32_t> treeUniformOffsets;
	// Get the hand positions from the hand system

	// Set transforms and sway params for tree instanced draw
	registry.Each<const Mesh, const Transform, const Tree, Swayable>(
	    [this, &treeUniformOffsets, drawBoundingBox](const Mesh& mesh, const Transform& transform, const Tree& tree,
	                                                 Swayable& swayable) {
		    auto offset = treeUniformOffsets.insert(std::make_pair(mesh.id, 0));
		    auto desc = _renderContext.treeInstancedDrawDescs.find(mesh.id);

		    const uint32_t idx = desc->second.offset + offset.first->second;
		    auto modelMatrix = glm::mat4(transform.rotation);
		    modelMatrix = glm::translate(modelMatrix, transform.position * transform.rotation);
		    modelMatrix = glm::scale(modelMatrix, transform.scale);
		    _renderContext.treeInstanceData[idx].modelMatrix = modelMatrix;
		    _renderContext.treeInstanceData[idx].swayParams =
		        glm::vec4(swayable.swayDirection.x, swayable.swayDirection.y, swayable.swayTime, swayable.swayStrength);

		    if (drawBoundingBox && idx + _renderContext.treeInstanceData.size() / 2 < _renderContext.treeInstanceData.size())
		    {
			    auto l3dMesh = entt::locator<resources::ResourcesInterface>::value().GetMeshes().Handle(mesh.id);
			    auto box = l3dMesh->GetBoundingBox();
			    auto boxMatrix = modelMatrix * glm::translate(box.Center()) * glm::scale(box.Size());

			    // Store bounding box matrix in the second half of the instance data array
			    _renderContext.treeInstanceData[idx + (_renderContext.treeInstanceData.size() / 2)].modelMatrix = boxMatrix;
		    }
		    offset.first->second++;
	    });

	if (!_renderContext.treeInstanceData.empty())
	{
		// Calculate the full buffer size including both matrices and sway params
		const auto size =
		    static_cast<uint32_t>(_renderContext.treeInstanceData.size() * (sizeof(glm::mat4) + sizeof(glm::vec4)));

		// Update the buffer with the combined data
		bgfx::update(_renderContext.treeInstanceUniformBuffer, 0, bgfx::makeRef(_renderContext.treeInstanceData.data(), size));
	}
}