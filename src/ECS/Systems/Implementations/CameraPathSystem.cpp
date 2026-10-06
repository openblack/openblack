/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
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

	_elapsed = std::chrono::microseconds::zero();
	_state = CameraPathState::PLAYING;
}

void CameraPathSystem::Stop()
{
	_state = CameraPathState::STOPPED;
	_elapsed = std::chrono::microseconds::zero();
	_path = entt::resource<CameraPath>();
}

void CameraPathSystem::Update(const std::chrono::microseconds& dt)
{
	if (_state == CameraPathState::STOPPED || !_path)
	{
		return;
	}
	if (_state != CameraPathState::PLAYING)
	{
		return;
	}

	// The path spreads its points evenly over its duration, and the camera follows it as the temple's does
	_elapsed += dt;
	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(_elapsed);
	const auto sample = _path->SampleAt(elapsed);
	auto& camera = Locator::camera::value();
	camera.SetOrigin(sample.position);
	camera.SetFocus(sample.focus);
	if (elapsed >= _path->GetDuration())
	{
		Stop();
	}
}
