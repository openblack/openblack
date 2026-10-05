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

#include <unordered_map>
#include <unordered_set>

#include <glm/gtx/transform.hpp>

#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
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
	// The game draws the room the player is in and the room the camera is on its way into. From the other rooms the
	// main room, which they lead off, is drawn whole only while one of its doors is open, and otherwise just its doors.
	const auto& temple = Locator::temple::value();
	_loadedRooms.clear();
	for (const auto room : {TempleRoom::Main, TempleRoom::CreatureCave, TempleRoom::Challenge, TempleRoom::Credits,
	                        TempleRoom::Multi, TempleRoom::Options, TempleRoom::SaveGame})
	{
		if (temple.IsRoomDrawn(room))
		{
			_loadedRooms.insert(room);
		}
	}
	const bool mainRoomDoorsOnly = !_loadedRooms.contains(TempleRoom::Main);
	if (mainRoomDoorsOnly)
	{
		_loadedRooms.insert(TempleRoom::Main);
	}

	auto prep = [&meshIds, &instanceCount](const Mesh& mesh, bool morphWithTerrain) {
		auto count = meshIds.insert(std::make_pair(mesh.id, std::make_pair(mesh.submeshId, morphWithTerrain)));
		count.first->second.first++;
		instanceCount++;
	};

	// The game mirrors the main room, without its floor or pool, through the plane of its origin and draws the
	// floor over the reflection, blended by the floor's alpha
	std::unordered_set<entt::id_type> mirroredMeshIds;
	std::unordered_set<entt::id_type> reflectiveMeshIds;
	std::unordered_set<entt::id_type> doorMeshIds;
	std::unordered_set<entt::id_type> otherRoomMeshIds;
	std::unordered_set<entt::id_type> waterMeshIds;
	std::unordered_set<entt::id_type> sideRoomMeshIds;
	std::unordered_map<entt::id_type, TempleRoom> roomMeshIds;
	const auto currentRoom = temple.GetCurrentRoom();
	registry.Each<const Mesh, const Transform, const TempleInteriorPart>(
	    [this, &prep, &mirroredMeshIds, &reflectiveMeshIds, &doorMeshIds, &otherRoomMeshIds, &waterMeshIds, &sideRoomMeshIds,
	     &roomMeshIds, mainRoomDoorsOnly,
	     currentRoom](const Mesh& mesh, const Transform& /* unused */, const TempleInteriorPart& templePart) {
		    if (!_loadedRooms.contains(templePart.room))
		    {
			    return;
		    }
		    if (templePart.room != currentRoom)
		    {
			    otherRoomMeshIds.insert(mesh.id);
		    }
		    if (templePart.mesh == TempleInteriorMesh::Water)
		    {
			    waterMeshIds.insert(mesh.id);
		    }
		    // The renderer draws the main room's pool itself, twice (Renderer::DrawTemplePool)
		    if (templePart.mesh == TempleInteriorMesh::Pool)
		    {
			    return;
		    }
		    if (templePart.room != TempleRoom::Main)
		    {
			    sideRoomMeshIds.insert(mesh.id);
		    }
		    if (templePart.mesh == TempleInteriorMesh::Room)
		    {
			    roomMeshIds.emplace(mesh.id, templePart.room);
		    }
		    if (templePart.room == TempleRoom::Main && mainRoomDoorsOnly)
		    {
			    // The game draws just the room's mesh, and of that just the doors
			    if (templePart.mesh == TempleInteriorMesh::Room)
			    {
				    prep(mesh, false);
				    doorMeshIds.insert(mesh.id);
			    }
			    return;
		    }
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
	// The game draws the player's hand in the temple too, while the hand is in its temple state
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
		drawDesc->second.onlyJoints = doorMeshIds.contains(meshId);
		// Each side room has its own copy of its door to the main room (the creature's room's "door arch03", joint 3),
		// which is lit by the room's lightmap and pokes up above the arch of the doorway. Drawn shut, in place, it shows
		// as a grey slab above the rotunda with a red edge, where the game shows nothing.
		// The game never shows it because it keeps the doors' joint matrices in the same table that skinned meshes
		// write their bones into: preparing the hand for drawing writes the hand's bones there, and the options room
		// sets the door matrices again after drawing the hand to put the doors back. The side room's door is presumably
		// drawn with a bone in its slot instead, which throws it out of its doorway.
		// TODO: which bone it is turned by, and where that puts it, needs the game run under a debugger. Until then the
		// side rooms' doors are drawn only while they swing.
		drawDesc->second.hideShutJoints = sideRoomMeshIds.contains(meshId);
		// The game draws each primitive of the temple's meshes by its material, blended or not, in their order. The
		// meshes of nothing but blended primitives, as the rooms' domes and floors are, go over the rest.
		if (meshId != Hand::k_MeshId)
		{
			drawDesc->second.materialBlending = true;
			const auto mesh = Locator::resources::value().GetMeshes().Handle(meshId);
			bool allBlended = true;
			for (const auto& subMesh : mesh->GetSubMeshes())
			{
				for (const auto& primitive : subMesh->GetPrimitives())
				{
					allBlended = allBlended && primitive.blend != decltype(primitive.blend)::Disabled;
				}
			}
			drawDesc->second.translucent = allBlended;
		}
		drawDesc->second.behindCurrentRoom = otherRoomMeshIds.contains(meshId);
		// The game draws each scroll with its own material, the texture its room wrote
		if (const auto room = roomMeshIds.find(meshId); room != roomMeshIds.end())
		{
			for (const auto& scroll : temple.GetScrollTextures(room->second))
			{
				drawDesc->second.subMeshTextures.emplace_back(scroll.subMesh, scroll.texture);
			}
			for (const auto& glow : temple.GetControlGlows(room->second))
			{
				drawDesc->second.subMeshGlows.emplace_back(glow.subMesh, glow.colour);
			}
			drawDesc->second.hiddenSubMeshes = temple.GetHiddenSubMeshes(room->second);
		}
		// The creature's room draws its water by the materials' alpha, sliding its texture down it
		if (waterMeshIds.contains(meshId))
		{
			drawDesc->second.translucent = true;
			drawDesc->second.uvOffset = temple.GetWaterfallSlide();
		}
		if (drawDesc->second.onlyJoints)
		{
			drawDesc->second.hiddenFromReflection = true;
		}
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

		    // The parts PrepareDrawDescs left out, as the main room's but its doors from the other rooms, have no draw
		    auto desc = _renderContext.instancedDrawDescs.find(mesh.id);
		    if (_loadedRooms.contains(templePart.room) && desc != _renderContext.instancedDrawDescs.end())
		    {
			    auto offset = uniformOffsets.insert(std::make_pair(mesh.id, 0));

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
