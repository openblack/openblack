/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreaturePlanner.h"

#include <algorithm>

using namespace openblack;
using namespace openblack::creature_planner;
using creature_desires::Desire;

namespace
{
/// The action's base priority, and how much each step of opinion adds
constexpr float k_BaseActionPriority = 0.5f;
constexpr float k_OpinionWeight = 0.0025f;
constexpr float k_NoveltyGrowth = 4.0f;
constexpr float k_FirstDesireNovelty = 0.025f;
} // namespace

float creature_planner::DistancePriority(bool held, float distance, float distanceWeight)
{
	if (held)
	{
		return k_HeldPriority;
	}
	return 1.0f - distanceWeight * std::min(distance, k_MaxGoalDistance) / k_MaxGoalDistance;
}

float creature_planner::ActionPriority(float opinion, bool disabled)
{
	return disabled ? 0.0f : k_BaseActionPriority + k_OpinionWeight * (opinion + 1.0f);
}

float creature_planner::Novelty(uint32_t turnsSinceDone, bool servesFirstDesire)
{
	const auto share = static_cast<float>(std::min(turnsSinceDone, k_NoveltyTurns)) / static_cast<float>(k_NoveltyTurns);
	return std::clamp(k_NoveltyGrowth * share * (servesFirstDesire ? k_FirstDesireNovelty : 1.0f), 0.0f, k_MaxNovelty);
}

float creature_planner::Priority(float desire, std::optional<float> goalUsefulness, float actionPriority,
                                 float objectUsefulness, std::optional<float> usedUsefulness)
{
	return desire * goalUsefulness.value_or(k_DefaultUsefulness) * std::max(actionPriority, k_MinActionPriority) *
	       objectUsefulness * usedUsefulness.value_or(k_DefaultUsefulness) * k_PriorityScale;
}

bool creature_planner::ShouldSwitch(float best, float current)
{
	return best - current > current;
}

std::optional<Plan> creature_planner::PlanDesire(Desire desire, float strength, std::span<const ObjectCandidate> objects,
                                                 float distanceWeight, std::span<const ActionCandidate> actions)
{
	// The goal: the thing that is most useful for the distance
	const ObjectCandidate* goal = nullptr;
	float goalScore = 0.0f;
	for (const auto& object : objects)
	{
		const auto score = DistancePriority(object.held, object.distance, distanceWeight) * object.usefulness;
		if (goal == nullptr || score > goalScore)
		{
			goal = &object;
			goalScore = score;
		}
	}
	// The action: the one most wanted, which nothing else is used for here
	const ActionCandidate* chosen = nullptr;
	float chosenPriority = 0.0f;
	float chosenScore = -1.0f;
	for (const auto& action : actions)
	{
		if (action.disabled || (action.needsObject && goal == nullptr))
		{
			continue;
		}
		const auto priority = ActionPriority(action.opinion) + Novelty(action.turnsSinceDone, action.servesFirstDesire);
		const auto score = k_DefaultUsefulness * k_DefaultUsefulness * priority;
		if (score >= chosenScore)
		{
			chosen = &action;
			chosenPriority = priority;
			chosenScore = score;
		}
	}
	if (chosen == nullptr)
	{
		return std::nullopt;
	}
	Plan plan {.desire = desire, .action = chosen->action, .actionPriority = chosenPriority};
	std::optional<float> goalUsefulness;
	if (chosen->needsObject && goal != nullptr)
	{
		plan.object = goal->id;
		plan.goalUsefulness = goal->usefulness;
		goalUsefulness = goal->usefulness;
	}
	plan.priority = Priority(strength, goalUsefulness, chosenPriority, k_DefaultUsefulness, std::nullopt);
	return plan;
}

std::vector<Desire> creature_planner::NextGoals(PlannerState& state,
                                                const std::array<bool, creature_desires::k_DesireCount>& eligible, size_t count)
{
	std::vector<Desire> goals;
	for (size_t tried = 0; tried < eligible.size() && goals.size() < count; ++tried)
	{
		const auto index = state.nextGoal % eligible.size();
		state.nextGoal = index + 1;
		if (eligible.at(index))
		{
			goals.push_back(static_cast<Desire>(index));
		}
		else
		{
			// A desire that can't be planned loses its old plan
			state.best.at(index).reset();
		}
	}
	return goals;
}

std::optional<Plan> creature_planner::Best(const PlannerState& state)
{
	std::optional<Plan> best;
	for (const auto& plan : state.best)
	{
		if (plan.has_value() && (!best.has_value() || plan->priority > best->priority))
		{
			best = plan;
		}
	}
	return best;
}

std::optional<Plan> creature_planner::Choose(const PlannerState& state, float minimum)
{
	const auto best = Best(state);
	if (!best.has_value() || best->priority < minimum)
	{
		return std::nullopt;
	}
	const auto current = state.current.has_value() ? state.current->priority : 0.0f;
	if (state.current.has_value() && state.current->desire == best->desire && state.current->action == best->action &&
	    state.current->object == best->object)
	{
		return std::nullopt;
	}
	return ShouldSwitch(best->priority, current) ? best : std::nullopt;
}
