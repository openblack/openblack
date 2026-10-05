/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <entt/core/fwd.hpp>
#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

namespace openblack::ecs::components
{

/// A sound kept up by an object or sounded at a point (see SoundTagSystemInterface).
///
/// An object's tag loops while it is switched on: it is started again each game turn it isn't playing and the camera
/// is within the sound's reach, and stops at once when switched off. A point's tag sounds once, after the sound has
/// had time to reach the camera when it is delayed, and goes when it has finished.
struct SoundTag
{
	entt::id_type sound;
	/// Where it sounds from, above the object's position
	glm::vec3 offset;
	bool active;
	/// The emitter playing it, if any
	entt::entity emitter {entt::null};
	/// A point's sound rather than an object's
	bool point {false};
	/// A point's sound still on its way to the camera
	bool delayed {false};
	/// Game turns since it was made
	uint16_t turns {0};
};

} // namespace openblack::ecs::components
