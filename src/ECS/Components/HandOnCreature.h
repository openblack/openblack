/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>

#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Creature/CreatureFeedback.h"

namespace openblack::ecs::components
{

/// On the player's hand while it is held to a creature: how it has stroked and slapped it so far
struct HandOnCreature
{
	entt::entity creature {entt::null};
	/// The running sum of strokes and slaps, from -1 to 1
	float sum {0.0f};
	/// How long the hand has rested on the body, and since the last stroke and slap, in milliseconds
	float onBodyMs {0.0f};
	float sinceStrokeMs {creature_feedback::k_StrokeIntervalMs};
	float sinceSlapMs {creature_feedback::k_SlapIntervalMs};
	/// The part last stroked
	std::optional<creature_feedback::BodyPart> lastPart;
	/// Where the hand was on the plane through the creature facing the camera, and the cursor, last frame
	std::optional<glm::vec3> lastPoint;
	glm::vec2 lastCursor {0.0f};
	/// How fast the hand moves, in units a second, eased a little over frames
	float speed {0.0f};
	/// The hand shows its slap for a moment after it slaps
	float slapShowMs {0.0f};
	/// Held by a command, such as a testbed scenario's, rather than by the button, which lets go only when told to
	bool byCommand {false};
};

/// On the player's hand once it has let go of a creature: the sum of strokes and slaps it let go with, which the
/// creature's status panel goes on showing until the hand next takes hold of a creature
struct HandLastFeedback
{
	float sum {0.0f};
};

} // namespace openblack::ecs::components
