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
#include "ECS/Components/AtHome.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureSpells.h"
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
#include "ECS/Components/Translucent.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Unlit.h"
#include "ECS/Registry.h"
#include "ECS/Systems/FieldSystemInterface.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "ECS/Systems/VegetationInterface.h"
#include "Game.h"
#include "Graphics/DebugLines.h"
#include "Graphics/GraphicsHandleBgfx.h"
#include "Graphics/ShaderManager.h"
#include "Locator.h"
#include "Profiler.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;

RenderingSystem::~RenderingSystem() = default;

void RenderingSystem::PrepareDrawDescs(bool drawBoundingBox)
{
	auto section = Locator::profiler::value().BeginScoped(Profiler::Stage::UpdateEntitiesDescs);
	auto& registry = Locator::entitiesRegistry::value();

	// Count number of instances. The draw lists are made from the components listed in k_ChangesDrawLayout, and made
	// again only when an entity gains or loses one of them: a component they come to depend on belongs in that list.
	uint32_t instanceCount = 0;
	struct MeshInstances
	{
		uint32_t count;
		bool morphWithTerrain;
		bool castsShadow;
		bool unlit;
		bool perEntity;
		bool translucent;
		std::optional<float> additiveShare;
	};
	std::unordered_map<entt::id_type, MeshInstances> meshIds;

	auto prep = [&registry, &meshIds, &instanceCount](entt::entity entity, const Mesh& mesh, bool morphWithTerrain) {
		auto count = meshIds.insert(std::make_pair(mesh.id, MeshInstances {.count = static_cast<uint32_t>(mesh.submeshId),
		                                                                   .morphWithTerrain = morphWithTerrain,
		                                                                   .castsShadow = false,
		                                                                   .unlit = false,
		                                                                   .perEntity = false,
		                                                                   .translucent = false,
		                                                                   .additiveShare = std::nullopt}));
		count.first->second.count++;
		// The things whose shadows Black & White bakes into the land (IsCastShadowAtNight), and its features
		count.first->second.castsShadow |= registry.AnyOf<Abode, Feature, MobileStatic, StoragePit>(entity);
		count.first->second.unlit |= registry.AnyOf<Unlit>(entity);
		count.first->second.perEntity |= registry.AnyOf<CreatureMorph>(entity);
		if (const auto* translucent = registry.TryGet<const Translucent>(entity))
		{
			count.first->second.translucent = true;
			count.first->second.additiveShare = translucent->share;
		}
		instanceCount++;
	};

	registry.Each<const Mesh, const Transform>(
	    [&prep](entt::entity entity, const Mesh& mesh, const Transform& /*unused*/) { prep(entity, mesh, false); },
	    entt::exclude<MorphWithTerrain, Tree, TempleInteriorPart, AtHome>);
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
		    .add(bgfx::Attrib::TexCoord3, 4, bgfx::AttribType::Float)
		    .end();
		_renderContext.instanceUniformBuffer = graphics::fromBgfx(bgfx::createDynamicVertexBuffer(instanceCount, layout));
		_renderContext.instanceUniforms.resize(instanceCount);
	}

	// Determine uniform buffer offsets and instance count for draw
	uint32_t offset = 0;
	_renderContext.instancedDrawDescs.clear();
	_instanceSlots.clear();
	for (const auto& [meshId, desc] : meshIds)
	{
		const auto [drawDesc, inserted] = _renderContext.instancedDrawDescs.emplace(
		    std::piecewise_construct, std::forward_as_tuple(meshId),
		    std::forward_as_tuple(offset, desc.count, desc.morphWithTerrain, desc.castsShadow));
		drawDesc->second.unlit = desc.unlit;
		drawDesc->second.perEntity = desc.perEntity;
		// Blended by its materials, over the opaque things
		drawDesc->second.materialBlending = desc.translucent;
		drawDesc->second.translucent = desc.translucent;
		drawDesc->second.additiveShare = desc.additiveShare;
		_instanceSlots.emplace(
		    meshId,
		    InstanceSlots {.offset = offset, .count = desc.count, .filled = 0, .perEntity = desc.perEntity, .height = 0.0f});
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
	_treeSlots.clear();
	auto& meshes = entt::locator<resources::ResourcesInterface>::value().GetMeshes();
	for (const auto& [meshId, count] : treeMeshIds)
	{
		_renderContext.treeInstancedDrawDescs.emplace(std::piecewise_construct, std::forward_as_tuple(meshId),
		                                              std::forward_as_tuple(treeOffset, count, false, true));
		const auto height = meshes.Contains(meshId) ? meshes.Handle(meshId)->GetBoundingBox().Size().y : 0.0f;
		_treeSlots.emplace(
		    meshId, InstanceSlots {.offset = treeOffset, .count = count, .filled = 0, .perEntity = false, .height = height});
		treeOffset += count;
	}
}

void RenderingSystem::PrepareDrawUploadUniforms(bool drawBoundingBox)
{
	// Fresh draw lists have room for every instance
	UploadInstances(drawBoundingBox);
	UploadTreeInstances(drawBoundingBox);
}

bool RenderingSystem::UploadUniformsKeepingDescs(bool drawBoundingBox)
{
	return UploadInstances(drawBoundingBox) && UploadTreeInstances(drawBoundingBox);
}

bool RenderingSystem::UploadInstances(bool drawBoundingBox)
{
	auto section = Locator::profiler::value().BeginScoped(Profiler::Stage::UpdateEntitiesUniforms);
	auto& registry = Locator::entitiesRegistry::value();

	for (auto& [meshId, slots] : _instanceSlots)
	{
		slots.filled = 0;
	}

	const auto& vegetation = Locator::vegetation::value();

	// Set transforms for instanced draw at offsets
	_renderContext.entityDraws.clear();
	bool fits = true;
	registry.Each<const Mesh, const Transform>(
	    [this, &registry, &vegetation, &fits, drawBoundingBox](entt::entity entity, const Mesh& mesh,
	                                                           const Transform& transform) {
		    // A mesh the draw lists don't have room for, which has changed since they were made
		    const auto slots = _instanceSlots.find(mesh.id);
		    if (!fits || slots == _instanceSlots.end() || slots->second.filled >= slots->second.count)
		    {
			    fits = false;
			    return;
		    }

		    auto modelMatrix = glm::mat4(transform.rotation);
		    modelMatrix = glm::translate(modelMatrix, transform.position * transform.rotation);
		    modelMatrix = glm::scale(modelMatrix, transform.scale);
		    // A home with someone in lights its windows at night
		    const auto* abode = registry.TryGet<const Abode>(entity);
		    glm::vec4 look {abode != nullptr && abode->presentAtHome > 0 ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f};
		    // A field's crop shows once it has grown a little, in its colour for how ripe it is, sunk into the ground by
		    // how empty it is, and sways once ripe
		    if (const auto* field = registry.TryGet<const Field>(entity); field != nullptr)
		    {
			    const auto crop = Locator::fieldSystem::value().GetLook(*field);
			    if (!crop.has_value())
			    {
				    look.z = 1.0f;
			    }
			    else
			    {
				    look.y = static_cast<float>(crop->tint);
				    look.w = field_crop::k_SnowCap;
				    const auto cropMesh = entt::locator<resources::ResourcesInterface>::value().GetMeshes().Handle(mesh.id);
				    const float cropHeight = cropMesh->GetBoundingBox().Size().y * transform.scale.y;
				    modelMatrix[3].y += field_crop::Sink(field->height.position) * cropHeight;
				    if (const auto* swayable = registry.TryGet<const Swayable>(entity); swayable != nullptr && crop->sways)
				    {
					    modelMatrix = vegetation.GetFieldMatrix(modelMatrix, transform.scale.y, swayable->swaySlot);
				    }
			    }
		    }

		    // A frozen creature takes an icy look, the land's light on it tinted blue as it freezes
		    // and an invisible one fizzes out of sight
		    if (const auto* spells = registry.TryGet<const CreatureSpells>(entity))
		    {
			    if (spells->freeze > 0.0f)
			    {
				    look.y = static_cast<float>(creature_spells::FrozenTint(spells->freeze));
			    }
			    if (spells->fizz > 0.0f)
			    {
				    look.z = -spells->fizz;
			    }
		    }

		    const uint32_t idx = slots->second.offset + slots->second.filled;
		    _renderContext.instanceUniforms[idx] = {.model = modelMatrix, .look = look};
		    if (slots->second.perEntity)
		    {
			    _renderContext.entityDraws.push_back({.entity = entity, .instance = idx});
		    }
		    if (drawBoundingBox)
		    {
			    auto l3dMesh = entt::locator<resources::ResourcesInterface>::value().GetMeshes().Handle(mesh.id);
			    auto box = l3dMesh->GetBoundingBox();
			    auto boxMatrix = modelMatrix * glm::translate(box.Center()) * glm::scale(box.Size());
			    _renderContext.instanceUniforms[idx + (_renderContext.instanceUniforms.size() / 2)] = {.model = boxMatrix};
		    }
		    ++slots->second.filled;
	    },
	    entt::exclude<TempleInteriorPart, Tree, AtHome>);

	if (fits && !_renderContext.instanceUniforms.empty())
	{
		const auto size = static_cast<uint32_t>(_renderContext.instanceUniforms.size() * sizeof(RenderContext::ObjectInstance));
		bgfx::update(toBgfx(_renderContext.instanceUniformBuffer), 0,
		             bgfx::makeRef(_renderContext.instanceUniforms.data(), size));
	}
	return fits;
}

bool RenderingSystem::UploadTreeInstances(bool drawBoundingBox)
{
	auto section = Locator::profiler::value().BeginScoped(Profiler::Stage::UpdateEntitiesTrees);
	auto& registry = Locator::entitiesRegistry::value();

	for (auto& [meshId, slots] : _treeSlots)
	{
		slots.filled = 0;
	}
	const auto& vegetation = Locator::vegetation::value();

	// Set the transforms of the trees, swaying or bent away from the hand
	bool fits = true;
	registry.Each<const Mesh, const Transform, const Tree, const Swayable>(
	    [this, &fits, drawBoundingBox, &vegetation](const Mesh& mesh, const Transform& transform, const Tree& /*unused*/,
	                                                const Swayable& swayable) {
		    const auto slots = _treeSlots.find(mesh.id);
		    if (!fits || slots == _treeSlots.end() || slots->second.filled >= slots->second.count)
		    {
			    fits = false;
			    return;
		    }

		    const uint32_t idx = slots->second.offset + slots->second.filled;
		    auto modelMatrix = glm::mat4(transform.rotation);
		    modelMatrix = glm::translate(modelMatrix, transform.position * transform.rotation);
		    modelMatrix = glm::scale(modelMatrix, transform.scale);
		    _renderContext.treeInstanceData[idx].modelMatrix = vegetation.GetTreeMatrix(
		        modelMatrix, transform.position, transform.scale.y, slots->second.height, swayable.swaySlot);

		    if (drawBoundingBox && idx + _renderContext.treeInstanceData.size() / 2 < _renderContext.treeInstanceData.size())
		    {
			    auto l3dMesh = entt::locator<resources::ResourcesInterface>::value().GetMeshes().Handle(mesh.id);
			    auto box = l3dMesh->GetBoundingBox();
			    auto boxMatrix = modelMatrix * glm::translate(box.Center()) * glm::scale(box.Size());

			    // Store bounding box matrix in the second half of the instance data array
			    _renderContext.treeInstanceData[idx + (_renderContext.treeInstanceData.size() / 2)].modelMatrix = boxMatrix;
		    }
		    ++slots->second.filled;
	    });

	if (fits && !_renderContext.treeInstanceData.empty())
	{
		const auto size =
		    static_cast<uint32_t>(_renderContext.treeInstanceData.size() * sizeof(RenderContext::TreeInstanceData));

		// Update the buffer with the combined data
		bgfx::update(toBgfx(_renderContext.treeInstanceUniformBuffer), 0,
		             bgfx::makeRef(_renderContext.treeInstanceData.data(), size));
	}
	return fits;
}
