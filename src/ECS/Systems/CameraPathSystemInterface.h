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

#include <entt/fwd.hpp>

namespace openblack::ecs::systems
{

class CameraPathSystemInterface
{
public:
	virtual void Start(entt::id_type id) = 0;
	virtual void Stop() = 0;
	virtual void Play() = 0;
	virtual void Pause() = 0;
	virtual void Update(const std::chrono::microseconds& dt) = 0;
	virtual bool IsPathing() = 0;
	virtual bool IsPaused() = 0;
};
} // namespace openblack::ecs::systems
