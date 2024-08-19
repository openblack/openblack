/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "CameraPathSystem.h"

#include <glm/gtx/euler_angles.hpp>

#include "Camera/Camera.h"
#include "ECS/Registry.h"
#include "Game.h"
#include "Locator.h"
#include "Resources/Resources.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;

void CameraPathSystem::Start(entt::id_type id)
{
	_path = Locator::resources::value().GetCameraPaths().Handle(id);
	if (!_path)
	{
		return;
	}

	_startingPosition = _path->GetPoints()[0].position;
	_currentStepCameraPosition = _path->GetPoints()[0].position;
	_currentStepLookAtPosition = _path->GetPoints()[0].rotation;
	_timeElapsedDuringStep = std::chrono::microseconds(0);
	_duration = _path->GetDuration() / _path->GetPoints().size();
	_state = CameraPathState::PLAYING;
	_pathIndex = 0;
}

void CameraPathSystem::Stop()
{
	if (_state == CameraPathState::STOPPED)
	{
		return;
	}

	_state = CameraPathState::STOPPED;
	_startingPosition = glm::vec3(0);
	_timeElapsedDuringStep = std::chrono::microseconds (0);
	_duration = std::chrono::microseconds (0);
	_state = CameraPathState::STOPPED;
	_pathIndex = 0;
	_path = entt::resource<CameraPath>();
}

void CameraPathSystem::Update(const std::chrono::microseconds& dt)
{
	if (_state == CameraPathState::STOPPED || (_path && (_pathIndex + 1) >= _path->GetPoints().size()))
	{
		Stop();
		return;
	}
	if (_state != CameraPathState::PLAYING)
	{
		return;
	}

	auto& camera = Locator::camera::value();
	const auto& currentStep = _path->GetPoints()[_pathIndex];
	const auto& nextStep = _path->GetPoints()[_pathIndex + 1];
	_timeElapsedDuringStep += dt;
	float blendFactor = std::chrono::duration<float, std::micro>(_timeElapsedDuringStep / _duration).count();
	blendFactor = glm::min(blendFactor, 1.f); // Avoid overstepping
	camera.SetOrigin(glm::lerp(currentStep.position, nextStep.position, blendFactor));
	camera.SetFocus(glm::lerp(currentStep.rotation, nextStep.rotation, blendFactor));

	// Move to the next step
	if (blendFactor >= 1.f)
	{
		_timeElapsedDuringStep = std::chrono::microseconds(0);
		_pathIndex++;
	}
}
