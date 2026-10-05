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
#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

namespace openblack::ecs::components
{

/// A looping sound an object keeps up while it is switched on (see SoundTagSystemInterface). It is started again each
/// game turn it isn't playing and the camera is within the sound's reach, and stops at once when switched off.
struct SoundTag
{
	entt::id_type sound;
	/// Where it sounds from, above the object's position
	glm::vec3 offset;
	bool active;
	/// The emitter playing it, if any
	entt::entity emitter {entt::null};
};

} // namespace openblack::ecs::components
