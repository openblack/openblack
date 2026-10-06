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

#include <entt/entity/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace openblack::ecs::systems
{

/// The player's hand on a creature (see components::HandOnCreature). The hand can't pick a creature up: clicking one
/// holds the hand to it. Resting on its body the hand strokes it, swept fast across it the hand slaps it, and the
/// creature reacts to each. When the hand lets go the creature's mind is told how it was treated, from -1 to 1.
class CreatureHandSystemInterface
{
public:
	/// Where the hand is drawn while held to a creature, and how it is posed
	struct HandPose
	{
		glm::vec3 position;
		/// Resting on the body rather than beside it
		bool onBody;
		/// Just slapped
		bool slapping;
	};

	virtual ~CreatureHandSystemInterface() = default;

	/// The hand's button was pressed with the cursor on a line of sight from a point: whether it is now held to a
	/// creature
	virtual bool Grab(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) = 0;
	/// Once a frame while held to a creature, with the line of sight through the cursor, the cursor on the screen and the
	/// frame's seconds: strokes and slaps, and where the hand is
	virtual std::optional<HandPose> Update(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec2 cursor,
	                                       float seconds) = 0;
	/// The button was let go: the creature is told how it was treated
	virtual void Release() = 0;

	/// The creature the hand is held to, if any
	[[nodiscard]] virtual std::optional<entt::entity> GetCreature() const = 0;
	/// How the creature has been treated since the hand took hold, from -1 to 1
	[[nodiscard]] virtual float GetFeedbackSum() const = 0;
};

} // namespace openblack::ecs::systems
