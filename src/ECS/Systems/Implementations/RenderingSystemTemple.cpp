/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "RenderingSystemTemple.h"

#include <glm/gtx/transform.hpp>

#include "3D/L3DMesh.h"
#include "Camera/Camera.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Stream.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Graphics/DebugLines.h"
#include "Graphics/ShaderManager.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;

RenderingSystemTemple::~RenderingSystemTemple() = default;

void RenderingSystemTemple::PrepareDrawDescs(bool drawBoundingBox)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto& camera = Locator::camera::value();

	// Count number of instances
	uint32_t instanceCount = 0;
	std::unordered_map<entt::id_type, std::pair<uint32_t, bool>> meshIds;
	std::set<TempleRoom> loadedRooms {TempleRoom::Main};
	auto roomLoaded = [&loadedRooms, &camera](const Mesh& mesh, const Transform& transform,
	                                          const TempleInteriorPart& templePart) {
		auto l3dMesh = entt::locator<resources::ResourcesInterface>::value().GetMeshes().Handle(mesh.id);
		auto box = l3dMesh->GetBoundingBox();
		auto cameraInsideRoom = box.Contains(camera.GetOrigin() - transform.position);
		if (cameraInsideRoom)
		{
			loadedRooms.emplace(templePart.room);
		}
	};
	registry.Each<const Mesh, const Transform, const TempleInteriorPart>(roomLoaded);
	_loadedRooms = loadedRooms;

	auto prep = [&meshIds, &instanceCount](const Mesh& mesh, bool morphWithTerrain) {
		auto count = meshIds.insert(std::make_pair(mesh.id, std::make_pair(mesh.submeshId, morphWithTerrain)));
		count.first->second.first++;
		instanceCount++;
	};

	registry.Each<const Mesh, const Transform, const TempleInteriorPart>(
	    [this, &prep](const Mesh& mesh, const Transform& /* unused */, const TempleInteriorPart& templePart) {
		    auto l3dMesh = entt::locator<resources::ResourcesInterface>::value().GetMeshes().Handle(mesh.id);
		    if (_loadedRooms.contains(templePart.room))
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

	if (!_renderContext.instanceUniforms.empty())
	{
		const auto size = static_cast<uint32_t>(_renderContext.instanceUniforms.size() * sizeof(glm::mat4));
		bgfx::update(_renderContext.instanceUniformBuffer, 0, bgfx::makeRef(_renderContext.instanceUniforms.data(), size));
	}
}
