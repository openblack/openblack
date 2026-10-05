/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <entt/core/fwd.hpp>
#include <entt/entity/fwd.hpp>
#include <glm/vec3.hpp>

namespace openblack::ecs::systems
{

/// The sounds objects keep up and that sound at points (components::SoundTag)
class SoundTagSystemInterface
{
public:
	virtual ~SoundTagSystemInterface() = default;
	/// Once a game turn: every switched on object's tag whose sound isn't playing starts it again, if the camera is
	/// within the sound's reach of it. A delayed point's sound plays once it has had time to reach the camera, if the
	/// camera is within its reach, and a point's tag goes once its sound has finished.
	virtual void ProcessTurn(const glm::vec3& camera) = 0;
	/// Switches an object's tag on or off; off stops its sound at once
	virtual void SetActive(entt::entity entity, bool active) = 0;
	/// Sounds once at a point; a delayed sound travels to the camera at the speed of sound first
	virtual entt::entity CreatePointSound(entt::id_type sound, const glm::vec3& position, bool delayed) = 0;
};

} // namespace openblack::ecs::systems
