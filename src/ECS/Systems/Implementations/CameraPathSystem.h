/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <chrono>
#include <deque>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "ECS/Systems/CameraPathSystemInterface.h"
#include "3D/CameraPath.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

enum class CameraPathState : uint8_t
{
	PLAYING,
	PAUSED,
	STOPPED
};

class CameraPathSystem final: public CameraPathSystemInterface
{
public:
	CameraPathSystem() : _startingPosition(0) {};
	void Start(entt::id_type id) override;
	void Stop() override;
	void Play() override { _state = CameraPathState::PLAYING; }
	void Pause() override { _state = CameraPathState::PAUSED; }
	void Update(const std::chrono::microseconds& dt) override;
	bool IsPathing() override { return _state == CameraPathState::PLAYING || _state == CameraPathState::PAUSED; }
	bool IsPaused() override { return _state == CameraPathState::PAUSED; }

private:
	entt::resource<CameraPath> _path;
	glm::vec3 _startingPosition;
	glm::vec3 _currentStepCameraPosition;
	glm::vec3 _currentStepLookAtPosition;
	std::chrono::microseconds _timeElapsedDuringStep;
	std::chrono::microseconds _duration;
	CameraPathState _state;
	int _pathIndex = 0;
};
} // namespace openblack::ecs::systems
