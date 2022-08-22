/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "CameraPathSystem.h"

#include <cassert>

#include <glm/gtx/euler_angles.hpp>

#include "3D/Camera.h"
#include "3D/CameraPath.h"
#include "ECS/Components/CameraPathNode.h"
#include "ECS/Registry.h"
#include "Game.h"
#include "Locator.h"
#include "Resources/Resources.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;

entt::entity CreateLookAtEntity(const glm::vec3& origin, const openblack::CameraPoint& cameraPoint)
{
	auto& registry = openblack::Game::Instance()->GetEntityRegistry();
	const auto lookAtEntity = registry.Create();
	const auto lookAtPosition = origin + cameraPoint.rotation;
	const auto lookAtRotation = glm::eulerAngleY(glm::radians(.0f));
	registry.Assign<openblack::ecs::components::Transform>(lookAtEntity, lookAtPosition, lookAtRotation, glm::vec3(1.0f));
	return lookAtEntity;
}

entt::entity CreatePositionEntity(const glm::vec3 origin, const openblack::CameraPoint& cameraPoint)
{
	auto& registry = openblack::Game::Instance()->GetEntityRegistry();
	const auto positionEntity = registry.Create();
	const auto currentPosition = origin + cameraPoint.position;
	const auto rotation = glm::eulerAngleY(glm::radians(.0f));
	registry.Assign<openblack::ecs::components::Transform>(positionEntity, currentPosition, rotation, glm::vec3(1.0f));
	return positionEntity;
}

entt::entity CreatePath(entt::id_type id, CameraPath& cameraPath)
{
	const auto& camera = Game::Instance()->GetCamera();
	const auto& cameraPos = camera.GetPosition();
	const auto& vertices = cameraPath.GetPoints();

	// Create a copy of the path
	auto& registry = Game::Instance()->GetEntityRegistry();

	// This is the start of the path and has an additional component
	entt::entity start = CreatePositionEntity(cameraPos, vertices[0]);
	const uint32_t flags = 0;
	registry.Assign<CameraPathStart>(start, id, flags);
	entt::entity previousPositionEntity = start;
	entt::entity previousLookAtEntity = CreateLookAtEntity(cameraPos, vertices[0]);

	for (size_t i = 1; i < vertices.size(); i++)
	{
		const auto& vertex = vertices[i];
		auto positionEntity = CreatePositionEntity(cameraPos, vertex);
		registry.Assign<CameraPathNode>(previousPositionEntity, positionEntity, previousLookAtEntity);
		previousPositionEntity = positionEntity;
		previousLookAtEntity = CreateLookAtEntity(cameraPos, vertex);
	}

	registry.Assign<ecs::components::CameraPathNode>(previousPositionEntity, entt::null, previousLookAtEntity);
	return start;
};

void DeletePath(entt::entity start)
{
	auto& registry = Game::Instance()->GetEntityRegistry();
	auto& startNode = registry.Get<ecs::components::CameraPathNode>(start);
	auto current = startNode.next;
	registry.Destroy(startNode.lookAt);

	while (current != entt::null)
	{
		auto node = registry.Get<ecs::components::CameraPathNode>(current);
		auto next = node.next;
		auto lookAt = node.lookAt;
		registry.Destroy(current);
		registry.Destroy(lookAt);
		current = next;
	}

	registry.Destroy(start);
}

void CameraPathSystem::Start(entt::id_type id)
{
	const auto& cameraPath = Locator::resources::value().GetCameraPaths().Handle(id);
	_start = CreatePath(id, cameraPath);
	_current = _start;
	_paused = false;
}

void CameraPathSystem::Stop(entt::entity start)
{
	DeletePath(start);
	_start = entt::null;
	_current = entt::null;
	_paused = false;
}

void CameraPathSystem::Update(const std::chrono::microseconds& dt)
{
	auto& registry = Game::Instance()->GetEntityRegistry();
	auto& camera = Game::Instance()->GetCamera();
	auto validStart = _start != entt::null && registry.Valid(_start);
	auto validCurrent = _current != entt::null && registry.Valid(_current);
	assert(validStart == validCurrent);

	if (!validStart)
	{
		return;
	}

	const auto& targetPosition = registry.Get<Transform>(_current).position;
	const auto& position = camera.GetPosition();
	if (position == targetPosition && !_paused)
	{
		const auto& node = registry.Get<CameraPathNode>(_current);

		_current = node.next;
		if (_current == entt::null)
		{
			if (registry.Valid(_start))
			{
				Stop(_start);
			}
		}
	}

	auto ms = camera.GetMaxSpeed();
	auto dtc = dt.count() * 10;
	_progress = glm::clamp(_progress + ms * dtc, .0f, 1.0f);
	camera.SetPosition(glm::mix(position, targetPosition, _progress));
}

bool CameraPathSystem::IsPlaying()
{
	auto& registry = Game::Instance()->GetEntityRegistry();
	return registry.Valid(_start);
}