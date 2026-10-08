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

#include <array>
#include <optional>
#include <string>

#include <entt/entity/entity.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

#include "Creature/CreatureObjectActions.h"
#include "Creature/CreatureReach.h"

namespace openblack::ecs::components
{

/// What a creature is doing with something, from walking up to it to the end of the animation that acts on it
struct CreatureObjectAction
{
	enum class Phase : uint8_t
	{
		/// Walking up to it or turning to it until it is in reach
		Approach,
		/// Playing the animations
		Playing,
	};
	creature_object_actions::Kind kind {creature_object_actions::Kind::PickUp};
	Phase phase {Phase::Approach};
	creature_object_actions::Status status {creature_object_actions::Status::Running};
	/// What it acts on, and the point it throws at or points at
	std::optional<entt::entity> target;
	glm::vec3 point {0.0f};

	/// Up to four animations played together by weight, all at the same time, played left to right or not
	std::array<size_t, 4> animations {};
	std::array<float, 4> weights {};
	uint8_t animationCount {0};
	bool mirrored {false};
	float timeMs {0.0f};
	float durationMs {0.0f};
	/// When it takes hold or lets go, in the animation's time, and whether that has happened
	float eventMs {0.0f};
	bool eventDone {false};
	/// Pointing loops its animations for a while rather than playing them once
	float holdMs {0.0f};

	/// Where the hand gets to in the reaching animations, and how far the creature can reach, measured as it starts
	std::optional<creature_reach::Points> reach;
	float maxReach {0.0f};
	/// How many times it has walked or turned to get in reach
	uint32_t attempts {0};
	/// Throwing: how long the throw is to take to get there
	float flightSeconds {0.0f};
	/// Why it gave up, when it did
	std::string failure;
};

/// What a creature holds in its hand, and which hand
/// The last thing a creature let go of into the physics, which the scripts can ask for and forget
struct CreatureDroppedObject
{
	entt::entity object {entt::null};
};

struct CreatureHeldObject
{
	entt::entity object {entt::null};
	/// Held in the left hand, having reached for it with the animations mirrored
	bool mirrored {false};
	/// The bone it rides on, and how it is turned against the creature
	uint32_t bone {0};
	glm::mat3 rotation {1.0f};
	/// Where its middle is from where it stands, in its own space at the world's scale, so it is held by its middle
	glm::vec3 middle {0.0f};
};

/// On something a creature holds: which creature
struct HeldByCreature
{
	entt::entity creature {entt::null};
};

/// How the creature's town sees what it is doing, which its villagers react to, and for how long more it will once
/// the creature stops
struct CreatureTownAttitude
{
	creature_object_actions::TownAttitude attitude {creature_object_actions::TownAttitude::None};
	float secondsLeft {0.0f};
};

} // namespace openblack::ecs::components
