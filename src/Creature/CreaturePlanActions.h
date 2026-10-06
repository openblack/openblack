/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include <glm/vec2.hpp>

#include "Creature/CreatureIdleMind.h"

/// The actions of the game's table that a creature can carry out here, and the agenda of steps each becomes. The planner
/// only weighs these; the rest (miracles, building, dancing and so on) wait for the systems they need.
namespace openblack::creature_plan_actions
{
/// The kind of thing an action is done to
enum class Target : uint8_t
{
	/// Nothing: done on the spot
	None,
	/// Anything it can eat, or only living things it can eat
	Food,
	LiveFood,
	/// Anything it can pick up
	Pickable,
	/// Anything it can knock down, or only trees
	Destroyable,
	Tree,
	/// Villagers, other creatures, or either
	Villager,
	Creature,
	Living,
	/// Anything at all it can see
	Anything,
};

/// How the agenda is made
enum class Build : uint8_t
{
	Eat,
	Sleep,
	Poo,
	Puke,
	Drink,
	ExamineByPickingUp,
	ExamineByLooking,
	ExamineByFollowing,
	ThrowAbout,
	Hurl,
	Destroy,
	SitDown,
	BeIdle,
	HangAround,
	ShowDesire,
	/// An action on the spot, or facing the player, or at what it goes up to
	Emote,
	FaceCameraEmote,
	ApproachEmote,
	RunFromObject,
	RunFromPlayer,
	LookAbout,
};

struct Executor
{
	/// The action's name in the game's table
	std::string_view action;
	Target target {Target::None};
	Build build {Build::Emote};
	/// The animation played, for the builds that play one
	size_t animation {0};
	creature_mind::Activity activity {creature_mind::Activity::Planned};
};

/// Every action that can be carried out
[[nodiscard]] std::span<const Executor> All();
[[nodiscard]] const Executor* For(std::string_view action);

/// What the agenda needs to know of where the creature is
struct Situation
{
	/// Where the player looks from, the nearest water's edge and the water, somewhere to hurl things at, and the action
	/// that shows its strongest desire
	std::optional<glm::vec2> camera;
	std::optional<creature_mind::Wants::WaterSpot> water;
	std::optional<glm::vec2> hurlTarget;
	std::optional<size_t> showDesireAnimation;
};
/// Whether an action can be carried out now, in this situation, before any thing is chosen
[[nodiscard]] bool Possible(const Executor& executor, const Situation& situation);
/// The agenda for an action on a thing (by its entity number, and where it is), if it can be done
[[nodiscard]] std::optional<std::vector<creature_mind::Step>> Agenda(const Executor& executor, std::optional<uint32_t> object,
                                                                     glm::vec2 objectPoint, const Situation& situation,
                                                                     const creature_mind::Random& random);

} // namespace openblack::creature_plan_actions
