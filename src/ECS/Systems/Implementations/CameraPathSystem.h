/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
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

#include "3D/CameraPath.h"
#include "ECS/Systems/CameraPathSystemInterface.h"

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
	void Start(entt::id_type id) override;
	void Stop() override;
	void Play() override { _state = CameraPathState::PLAYING; }
	void Pause() override { _state = CameraPathState::PAUSED; }
	void Update(const std::chrono::microseconds& dt) override;
	bool IsPathing() override { return _state == CameraPathState::PLAYING || _state == CameraPathState::PAUSED; }
	bool IsPaused() override { return _state == CameraPathState::PAUSED; }

private:
	entt::resource<CameraPath> _path;
	/// How far along the path the camera is
	std::chrono::microseconds _elapsed {0};
	CameraPathState _state {CameraPathState::STOPPED};
};
} // namespace openblack::ecs::systems
