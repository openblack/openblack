/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "RenderingSystemTemple.h"

#include <unordered_set>

#include <glm/gtx/transform.hpp>

#include "3D/L3DMesh.h"
#include "3D/TempleInteriorInterface.h"
#include "Camera/Camera.h"
#include "ECS/Components/Hand.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Stream.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "Graphics/DebugLines.h"
#include "Graphics/GraphicsHandleBgfx.h"
#include "Graphics/ShaderManager.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;

namespace
{
/// The hand of the player, whose hand alone is in the temple. The other hand waits at the world's origin, where the
/// temple's rooms are.
entt::entity PlayerHand()
{
	if (!openblack::Locator::handSystem::has_value())
	{
		return entt::null;
	}
	return openblack::Locator::handSystem::value().GetPlayerHands()[static_cast<size_t>(HandSystemInterface::Side::Left)];
}
} // namespace

RenderingSystemTemple::~RenderingSystemTemple() = default;

void RenderingSystemTemple::PrepareDrawDescs(bool drawBoundingBox)
{
	auto& registry = Locator::entitiesRegistry::value();

	// Count number of instances
	uint32_t instanceCount = 0;
	std::unordered_map<entt::id_type, std::pair<uint32_t, bool>> meshIds;
	// Temple::Draw draws the room the player is in and the main room, which the others lead off, and the room the
	// camera is on its way into
	const auto& temple = Locator::temple::value();
	_loadedRooms = {TempleRoom::Main, temple.GetCurrentRoom()};
	if (const auto transition = temple.GetTransitionRoom(); transition.has_value())
	{
		_loadedRooms.insert(*transition);
	}

	auto prep = [&meshIds, &instanceCount](const Mesh& mesh, bool morphWithTerrain) {
		auto count = meshIds.insert(std::make_pair(mesh.id, std::make_pair(mesh.submeshId, morphWithTerrain)));
		count.first->second.first++;
		instanceCount++;
	};

	// WorldRoom::Draw mirrors the main room, without its floor or pool, through the plane of its origin and draws the
	// floor over the reflection, blended by the floor's alpha
	std::unordered_set<entt::id_type> mirroredMeshIds;
	std::unordered_set<entt::id_type> reflectiveMeshIds;
	registry.Each<const Mesh, const Transform, const TempleInteriorPart>(
	    [this, &prep, &mirroredMeshIds, &reflectiveMeshIds](const Mesh& mesh, const Transform& /* unused */,
	                                                        const TempleInteriorPart& templePart) {
		    if (_loadedRooms.contains(templePart.room))
		    {
			    prep(mesh, false);
			    if (templePart.room == TempleRoom::Main && templePart.mesh == TempleInteriorMesh::Room)
			    {
				    mirroredMeshIds.insert(mesh.id);
			    }
			    else if (templePart.room == TempleRoom::Main && templePart.mesh == TempleInteriorMesh::Floor)
			    {
				    reflectiveMeshIds.insert(mesh.id);
			    }
		    }
	    });
	// CHand draws the player's hand in the temple too, in HandStateCitadel
	const auto playerHand = PlayerHand();
	registry.Each<const Mesh, const Transform, const Hand>(
	    [&prep, playerHand](const entt::entity entity, const Mesh& mesh, const Transform& /*unused*/, const Hand& /*unused*/) {
		    if (entity == playerHand)
		    {
			    prep(mesh, false);
		    }
	    });

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
		auto [drawDesc, _] =
		    _renderContext.instancedDrawDescs.emplace(std::piecewise_construct, std::forward_as_tuple(meshId),
		                                              std::forward_as_tuple(offset, desc.first, desc.second, false));
		drawDesc->second.hiddenFromReflection = !mirroredMeshIds.contains(meshId);
		drawDesc->second.showsReflection = reflectiveMeshIds.contains(meshId);
		offset += desc.first;
	}
}

void RenderingSystemTemple::PrepareDrawUploadUniforms(bool drawBoundingBox)
{
	auto& registry = Locator::entitiesRegistry::value();

	// Store offsets of uniforms for descs
	std::map<entt::id_type, uint32_t> uniformOffsets;

	// Set transforms for instanced draw at offsets
	registry.Each<const Mesh, const Transform, const TempleInteriorPart>(
	    [this, &uniformOffsets, drawBoundingBox](const Mesh& mesh, const Transform& transform,
	                                             const TempleInteriorPart& templePart) {
		    auto l3dMesh = entt::locator<resources::ResourcesInterface>::value().GetMeshes().Handle(mesh.id);

		    if (_loadedRooms.contains(templePart.room))
		    {
			    auto offset = uniformOffsets.insert(std::make_pair(mesh.id, 0));
			    auto desc = _renderContext.instancedDrawDescs.find(mesh.id);

			    auto modelMatrix = glm::mat4(transform.rotation);
			    modelMatrix = glm::translate(modelMatrix, transform.position * transform.rotation);
			    modelMatrix = glm::scale(modelMatrix, transform.scale);

			    const uint32_t idx = desc->second.offset + offset.first->second;
			    _renderContext.instanceUniforms[idx] = modelMatrix;
			    if (drawBoundingBox)
			    {
				    auto box = l3dMesh->GetBoundingBox();
				    auto boxMatrix = modelMatrix * glm::translate(box.Center()) * glm::scale(box.Size());
				    _renderContext.instanceUniforms[idx + _renderContext.instanceUniforms.size() / 2] = boxMatrix;
			    }
			    offset.first->second++;
		    }
	    });
	registry.Each<const Mesh, const Transform, const Hand>(
	    [this, &uniformOffsets, playerHand = PlayerHand()](const entt::entity entity, const Mesh& mesh,
	                                                       const Transform& transform, const Hand& /*unused*/) {
		    if (entity != playerHand)
		    {
			    return;
		    }
		    auto offset = uniformOffsets.insert(std::make_pair(mesh.id, 0));
		    const auto desc = _renderContext.instancedDrawDescs.find(mesh.id);
		    auto modelMatrix = glm::mat4(transform.rotation);
		    modelMatrix = glm::translate(modelMatrix, transform.position * transform.rotation);
		    modelMatrix = glm::scale(modelMatrix, transform.scale);
		    _renderContext.instanceUniforms[desc->second.offset + offset.first->second] = modelMatrix;
		    offset.first->second++;
	    });

	if (!_renderContext.instanceUniforms.empty())
	{
		const auto size = static_cast<uint32_t>(_renderContext.instanceUniforms.size() * sizeof(glm::mat4));
		bgfx::update(toBgfx(_renderContext.instanceUniformBuffer), 0,
		             bgfx::makeRef(_renderContext.instanceUniforms.data(), size));
	}
}
