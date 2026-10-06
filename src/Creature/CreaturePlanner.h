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

#include <array>
#include <optional>
#include <span>
#include <vector>

#include "Creature/CreatureDesires.h"

/// How a creature decides what to do. Each desire it has gets a plan: the thing to act on (the goal), and the action to
/// take. The goal is the thing the creature finds most useful for the desire, by what its decision trees have learnt,
/// the nearer the better. Among the actions that satisfy the desire, its opinion of each and how long since it last did
/// it break the ties. A plan's priority is the desire's strength times how useful the goal and the action are. Two
/// desires are planned again each game turn, and the creature changes what it is doing only for a plan more than twice
/// as pressing as the one it carries out.
namespace openblack::creature_planner
{
/// The farthest a goal counts as being, and how much a held thing is preferred
constexpr float k_MaxGoalDistance = 200.0f;
constexpr float k_HeldPriority = 1.6f;
/// How much a thing nothing is known about is worth, and the floor of an action's priority in a plan
constexpr float k_DefaultUsefulness = 0.1f;
constexpr float k_MinActionPriority = 0.01f;
/// Scales priorities to readable numbers; it doesn't change which plan wins
constexpr float k_PriorityScale = 1e5f;
/// How long since an action was done before it is fully new again, in game turns, and the most novelty adds
constexpr uint32_t k_NoveltyTurns = 36000;
constexpr float k_MaxNovelty = 0.01f;
/// Desires planned each game turn
constexpr size_t k_GoalsPerTurn = 2;

/// How the distance to a goal weighs against it: 1.6 for something held, else less the further it is, by the desire's
/// distance weight, up to 200 away
[[nodiscard]] float DistancePriority(bool held, float distance, float distanceWeight);
/// How much the creature wants to take an action, by its opinion of it from -1 to 1; nothing for an action it can't
/// take now
[[nodiscard]] float ActionPriority(float opinion, bool disabled = false);
/// A little extra for an action not done for a long time
[[nodiscard]] float Novelty(uint32_t turnsSinceDone, bool servesFirstDesire);
/// A plan's priority from its parts: the desire's strength, the goal's usefulness (or none), the action's priority, and
/// the usefulness of what is acted on and of what is used (or none)
[[nodiscard]] float Priority(float desire, std::optional<float> goalUsefulness, float actionPriority, float objectUsefulness,
                             std::optional<float> usedUsefulness);
/// Whether a plan is pressing enough to change to: more than twice the priority of what the creature carries out
[[nodiscard]] bool ShouldSwitch(float best, float current);

/// Something a desire could be satisfied with
struct ObjectCandidate
{
	/// The thing's entity number
	uint32_t id {0};
	float distance {0.0f};
	bool held {false};
	/// How useful the creature has learnt it is for the desire
	float usefulness {k_DefaultUsefulness};
};

/// An action that satisfies a desire, by its row in the game's action table
struct ActionCandidate
{
	uint32_t action {0};
	/// Whether it acts on a thing, so needs a goal
	bool needsObject {false};
	float opinion {0.0f};
	uint32_t turnsSinceDone {k_NoveltyTurns};
	/// Whether the table says it serves the first desire, which the game counts for less novelty
	bool servesFirstDesire {false};
	bool disabled {false};
};

struct Plan
{
	creature_desires::Desire desire {creature_desires::Desire::Impress};
	uint32_t action {0};
	std::optional<uint32_t> object;
	float goalUsefulness {k_DefaultUsefulness};
	float actionPriority {0.0f};
	float priority {0.0f};
};

/// The best plan for a desire of a strength: the goal by distance and usefulness, then the action most wanted
[[nodiscard]] std::optional<Plan> PlanDesire(creature_desires::Desire desire, float strength,
                                             std::span<const ObjectCandidate> objects, float distanceWeight,
                                             std::span<const ActionCandidate> actions);

/// What the planner holds between turns
struct PlannerState
{
	/// The best plan found for each desire, the last time it was planned
	std::array<std::optional<Plan>, creature_desires::k_DesireCount> best {};
	/// The desire planned next
	size_t nextGoal {0};
	/// The plan carried out
	std::optional<Plan> current;
};
/// The next desires to plan this turn, going round those that may be planned
[[nodiscard]] std::vector<creature_desires::Desire> NextGoals(PlannerState& state,
                                                              const std::array<bool, creature_desires::k_DesireCount>& eligible,
                                                              size_t count = k_GoalsPerTurn);
/// The most pressing plan of all the desires', if any
[[nodiscard]] std::optional<Plan> Best(const PlannerState& state);
/// The plan to change to now, if the best is pressing enough against what the creature carries out (nothing counts as 0)
/// and at least a minimum priority
[[nodiscard]] std::optional<Plan> Choose(const PlannerState& state, float minimum);

} // namespace openblack::creature_planner
