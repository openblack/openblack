/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
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
#include <unordered_map>

#include <glm/gtx/transform.hpp>

#include "3D/L3DMesh.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Feature.h"
#include "ECS/Components/Field.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/MorphWithTerrain.h"
#include "ECS/Components/StoragePit.h"
#include "ECS/Components/Stream.h"
#include "ECS/Components/Swayable.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Unlit.h"
#include "ECS/Registry.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "ECS/Systems/VegetationInterface.h"
#include "Game.h"
#include "Graphics/DebugLines.h"
#include "Graphics/GraphicsHandleBgfx.h"
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
	struct MeshInstances
	{
		uint32_t count;
		bool morphWithTerrain;
		bool castsShadow;
		bool unlit;
	};
	std::unordered_map<entt::id_type, MeshInstances> meshIds;

	auto prep = [&registry, &meshIds, &instanceCount](entt::entity entity, const Mesh& mesh, bool morphWithTerrain) {
		auto count = meshIds.insert(std::make_pair(mesh.id, MeshInstances {.count = static_cast<uint32_t>(mesh.submeshId),
		                                                                   .morphWithTerrain = morphWithTerrain,
		                                                                   .castsShadow = false,
		                                                                   .unlit = false}));
		count.first->second.count++;
		// The things whose shadows Black & White bakes into the land (IsCastShadowAtNight), and its features
		count.first->second.castsShadow |= registry.AnyOf<Abode, Feature, MobileStatic, StoragePit>(entity);
		count.first->second.unlit |= registry.AnyOf<Unlit>(entity);
		instanceCount++;
	};

	registry.Each<const Mesh, const Transform>(
	    [&prep](entt::entity entity, const Mesh& mesh, const Transform& /*unused*/) { prep(entity, mesh, false); },
	    entt::exclude<MorphWithTerrain, Tree, TempleInteriorPart>);
	registry.Each<const Mesh, const Transform, const MorphWithTerrain>(
	    [&prep](entt::entity entity, const Mesh& mesh, const Transform& /*unused*/, const MorphWithTerrain& /*unused*/) {
		    prep(entity, mesh, true);
	    },
	    entt::exclude<Tree>);

	if (drawBoundingBox)
	{
		instanceCount *= 2;
	}

	// Recreate instancing uniform buffer if it is too small
	if (_renderContext.instanceUniforms.size() < instanceCount)
	{
		if (bgfx::isValid(toBgfx(_renderContext.instanceUniformBuffer)))
		{
			bgfx::destroy(toBgfx(_renderContext.instanceUniformBuffer));
		}
		bgfx::VertexLayout layout;
		layout.begin()
		    .add(bgfx::Attrib::TexCoord7, 4, bgfx::AttribType::Float)
		    .add(bgfx::Attrib::TexCoord6, 4, bgfx::AttribType::Float)
		    .add(bgfx::Attrib::TexCoord5, 4, bgfx::AttribType::Float)
		    .add(bgfx::Attrib::TexCoord4, 4, bgfx::AttribType::Float)
		    .end();
		_renderContext.instanceUniformBuffer = graphics::fromBgfx(bgfx::createDynamicVertexBuffer(instanceCount, layout));
		_renderContext.instanceUniforms.resize(instanceCount);
	}

	// Determine uniform buffer offsets and instance count for draw
	uint32_t offset = 0;
	_renderContext.instancedDrawDescs.clear();
	for (const auto& [meshId, desc] : meshIds)
	{
		const auto [drawDesc, inserted] = _renderContext.instancedDrawDescs.emplace(
		    std::piecewise_construct, std::forward_as_tuple(meshId),
		    std::forward_as_tuple(offset, desc.count, desc.morphWithTerrain, desc.castsShadow));
		drawDesc->second.unlit = desc.unlit;
		offset += desc.count;
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

	_renderContext.treeInstanceCount = treeInstanceCount;
	if (drawBoundingBox)
	{
		treeInstanceCount *= 2;
	}

	// Create tree instance buffer if needed
	if (_renderContext.treeInstanceData.size() < treeInstanceCount)
	{
		if (bgfx::isValid(toBgfx(_renderContext.treeInstanceUniformBuffer)))
		{
			bgfx::destroy(toBgfx(_renderContext.treeInstanceUniformBuffer));
		}
		bgfx::VertexLayout layout;
		layout.begin()
		    .add(bgfx::Attrib::TexCoord7, 4, bgfx::AttribType::Float) // i_data0 (matrix row 0)
		    .add(bgfx::Attrib::TexCoord6, 4, bgfx::AttribType::Float) // i_data1 (matrix row 1)
		    .add(bgfx::Attrib::TexCoord5, 4, bgfx::AttribType::Float) // i_data2 (matrix row 2)
		    .add(bgfx::Attrib::TexCoord4, 4, bgfx::AttribType::Float) // i_data3 (matrix row 3)
		    .end();
		_renderContext.treeInstanceUniformBuffer =
		    graphics::fromBgfx(bgfx::createDynamicVertexBuffer(treeInstanceCount, layout));
		_renderContext.treeInstanceData.resize(treeInstanceCount);
	}

	// Determine tree uniform buffer offsets and instance count for draw
	uint32_t treeOffset = 0;
	_renderContext.treeInstancedDrawDescs.clear();
	for (const auto& [meshId, count] : treeMeshIds)
	{
		_renderContext.treeInstancedDrawDescs.emplace(std::piecewise_construct, std::forward_as_tuple(meshId),
		                                              std::forward_as_tuple(treeOffset, count, false, true));
		treeOffset += count;
	}
}

void RenderingSystem::PrepareDrawUploadUniforms(bool drawBoundingBox)
{
	auto& registry = Locator::entitiesRegistry::value();

	// Store offsets of uniforms for descs
	std::map<entt::id_type, uint32_t> uniformOffsets;

	const auto& vegetation = Locator::vegetation::value();

	// Set transforms for instanced draw at offsets
	registry.Each<const Mesh, const Transform>(
	    [this, &registry, &vegetation, &uniformOffsets, drawBoundingBox](entt::entity entity, const Mesh& mesh,
	                                                                     const Transform& transform) {
		    auto offset = uniformOffsets.insert(std::make_pair(mesh.id, 0));
		    auto desc = _renderContext.instancedDrawDescs.find(mesh.id);

		    auto modelMatrix = glm::mat4(transform.rotation);
		    modelMatrix = glm::translate(modelMatrix, transform.position * transform.rotation);
		    modelMatrix = glm::scale(modelMatrix, transform.scale);
		    // Fields' crops sway
		    if (const auto* swayable = registry.TryGet<const Swayable>(entity);
		        swayable != nullptr && registry.AnyOf<Field>(entity))
		    {
			    modelMatrix = vegetation.GetFieldMatrix(modelMatrix, transform.scale.y, swayable->swaySlot);
		    }

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
		bgfx::update(toBgfx(_renderContext.instanceUniformBuffer), 0,
		             bgfx::makeRef(_renderContext.instanceUniforms.data(), size));
	}

	// Upload tree uniforms separately
	PrepareTreeDrawUploadUniforms(drawBoundingBox);
}

void RenderingSystem::PrepareTreeDrawUploadUniforms(bool drawBoundingBox)
{
	auto& registry = Locator::entitiesRegistry::value();

	// Store offsets of tree uniforms
	std::map<entt::id_type, uint32_t> treeUniformOffsets;
	const auto& vegetation = Locator::vegetation::value();
	auto& meshes = entt::locator<resources::ResourcesInterface>::value().GetMeshes();

	// Set the transforms of the trees, swaying or bent away from the hand
	registry.Each<const Mesh, const Transform, const Tree, const Swayable>(
	    [this, &treeUniformOffsets, drawBoundingBox, &vegetation, &meshes](const Mesh& mesh, const Transform& transform,
	                                                                       const Tree& /*unused*/, const Swayable& swayable) {
		    auto offset = treeUniformOffsets.insert(std::make_pair(mesh.id, 0));
		    auto desc = _renderContext.treeInstancedDrawDescs.find(mesh.id);

		    const uint32_t idx = desc->second.offset + offset.first->second;
		    auto modelMatrix = glm::mat4(transform.rotation);
		    modelMatrix = glm::translate(modelMatrix, transform.position * transform.rotation);
		    modelMatrix = glm::scale(modelMatrix, transform.scale);
		    const auto height = meshes.Contains(mesh.id) ? meshes.Handle(mesh.id)->GetBoundingBox().Size().y : 0.0f;
		    _renderContext.treeInstanceData[idx].modelMatrix =
		        vegetation.GetTreeMatrix(modelMatrix, transform.position, transform.scale.y, height, swayable.swaySlot);

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
		const auto size =
		    static_cast<uint32_t>(_renderContext.treeInstanceData.size() * sizeof(RenderContext::TreeInstanceData));

		// Update the buffer with the combined data
		bgfx::update(toBgfx(_renderContext.treeInstanceUniformBuffer), 0,
		             bgfx::makeRef(_renderContext.treeInstanceData.data(), size));
	}
}
