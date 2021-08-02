/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "CameraSceneSystem.h"
#include "Game.h"
#include "3D/Camera.h"

using namespace openblack::entities::systems;
using namespace openblack::entities::components;

void CameraSceneSystem::CameraPointCreated(entt::registry& registry, entt::entity entity)
{
	// This system can handle multiple camera paths. Paths are re-played in the order they are assigned within the ECS
	// Camera points are effectively nodes.
	// 1. Camera nodes are linked together by ECS entity IDs. They form a one-way linked-list
	// 1. Each camera path has a node that has "start" set to true
	// 2. These nodes fulfill the condition below and are added to the starting points list
	// 3. We follow the linked-list of nodes from the "start" node to its final node. This is done in Update()
	// 4. Once we hit ECS ID == null (end of the linked-list), we check if another start node exists
	// 5. If another one exists then we start the whole process again from 3 until the starting points list is empty
	if (registry.get<CameraPoint>(entity).start)
	{
		_startingPoints.emplace_back(entity);
	}
}

void CameraSceneSystem::Connect(openblack::entities::Registry& registry)
{
	// Attach a listener to the ECS. We are interested in newly assigned camera point components as they indicate
	// the creation of a new camera path that we must follow
	registry.OnConstruct<CameraPoint>().connect<&CameraSceneSystem::CameraPointCreated>(*this);
}

bool CameraSceneSystem::Update(std::chrono::duration<float, std::milli> dt)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	auto& camera = Game::instance()->GetCamera();

	// The starting point for our camera path may have been deleted since we added it to our list of starts
	// This can happen if the ECS was cleared of all its entities or the camera path entities were removed
	while (!registry.Valid(_currentPoint) && !_startingPoints.empty())
	{
		_progress = 0;
		_currentPosition = Game::instance()->GetCamera().GetPosition();
		_currentPoint = _startingPoints.front();
		_startingPoints.pop_front();
	}

	// No valid point to create a new position from
	if (!registry.Valid(_currentPoint) || _currentPoint == entt::null)
	{
		return true;
	}

	auto activeCamTarget = registry.Get<CameraPoint>(_currentPoint);
	auto ms = camera.GetMaxSpeed();

	// TODO(raffclar): The movement speed is affected by the number of nodes. We need to work on the mechanism that
	//  re-plays a camera path and compensates for delta-time
	if (activeCamTarget.start)
	{
		ms /= 100;
	}

	// Basic attempt at checking if we've reached the target position
	auto targetPosition = activeCamTarget.position;
	auto dtc = dt.count() * 10;
	_progress = glm::clamp(_progress + ms * dtc, .0f, 1.0f);
	auto position = glm::mix(_currentPosition, targetPosition, _progress);
	camera.SetPosition(position);

	if (_progress >= 1.0f)
	{
		_progress = 0;
		_currentPosition = camera.GetPosition();
		auto previousPoint = _currentPoint;
		_currentPoint = activeCamTarget.next;
		registry.Destroy(previousPoint);
	}

	return true;
}
