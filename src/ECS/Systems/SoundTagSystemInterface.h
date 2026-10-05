/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <entt/entity/fwd.hpp>
#include <glm/vec3.hpp>

namespace openblack::ecs::systems
{

/// The looping sounds objects keep up (components::SoundTag)
class SoundTagSystemInterface
{
public:
	virtual ~SoundTagSystemInterface() = default;
	/// Once a game turn: every switched on tag whose sound isn't playing starts it again, if the camera is within the
	/// sound's reach of it
	virtual void ProcessTurn(const glm::vec3& camera) = 0;
	/// Switches an object's tag on or off; off stops its sound at once
	virtual void SetActive(entt::entity entity, bool active) = 0;
};

} // namespace openblack::ecs::systems
