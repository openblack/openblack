/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <chrono>
#include <deque>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "ECS/Systems/CameraPathSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class CameraPathSystem final: public CameraPathSystemInterface
{
public:
	CameraPathSystem()
	    : _start(entt::null)
	    , _current(entt::null) {};
	void Start(entt::id_type id) override;
	void Stop(entt::entity start) override;
	void Pause(bool flag) override { _paused = flag; };
	void Update(const std::chrono::microseconds& dt) override;
	bool IsPlaying() override;
	bool IsPaused() override { return _paused; };

private:
	entt::entity _start;
	entt::entity _current;
	bool _paused = false;
	float _progress;
};
} // namespace openblack::ecs::systems
