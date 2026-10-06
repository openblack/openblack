/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "LeashKeys.h"

#include <cmath>

#include <algorithm>

using namespace openblack;
using namespace openblack::creature_leash;
using input::BindableActionMap;

std::optional<LeashKey> creature_leash::KeyFor(BindableActionMap action)
{
	switch (action)
	{
	case BindableActionMap::LEASH_UNLEASH_CREATURE:
		return LeashKey::Leash;
	case BindableActionMap::PREVIOUS_LEASH:
		return LeashKey::PreviousLeash;
	case BindableActionMap::NEXT_LEASH:
		return LeashKey::NextLeash;
	default:
		return std::nullopt;
	}
}

std::optional<LeashType> creature_leash::StepType(LeashType selected, const std::bitset<k_Types.size()>& known, bool up)
{
	// k_Types lists the leashes in number order, so one number up is one place on
	static_assert(k_Types[0] == LeashType::Evil && k_Types[1] == LeashType::Rope && k_Types[2] == LeashType::Good);
	const auto count = k_Types.size();
	// From the learning leash when none is picked
	const auto from = IndexOf(selected).value_or(*IndexOf(LeashType::Rope));
	const auto to = up ? (from + 1) % count : (from + count - 1) % count;
	return known.test(to) ? std::optional(k_Types.at(to)) : std::nullopt;
}

KeyCommand creature_leash::CommandFor(LeashKey key, const KeyState& state)
{
	using Kind = KeyCommand::Kind;
	const auto knows = [&state](LeashType type) {
		const auto index = IndexOf(type);
		return index.has_value() && state.known.test(*index);
	};
	switch (key)
	{
	case LeashKey::Leash:
		if (state.worn && state.tied)
		{
			return {.kind = Kind::UntieToHand};
		}
		if (state.worn)
		{
			return {.kind = Kind::TakeOff};
		}
		if (!knows(LeashType::Rope))
		{
			return {};
		}
		return {.kind = Kind::PutOn, .type = knows(state.selected) ? state.selected : LeashType::Rope};
	case LeashKey::PreviousLeash:
	case LeashKey::NextLeash:
		if (const auto next = StepType(state.selected, state.known, key == LeashKey::PreviousLeash))
		{
			return {.kind = Kind::ChangeType, .type = *next};
		}
		return {};
	}
	return {};
}

bool creature_leash::TrackShake(ShakeTracker& tracker, glm::vec2 cursor, float seconds)
{
	tracker.clock += std::max(seconds, 0.0f);
	if (!tracker.started)
	{
		tracker.started = true;
		for (size_t axis = 0; axis < tracker.axes.size(); ++axis)
		{
			tracker.axes.at(axis) = {.direction = 0,
			                         .start = cursor[static_cast<glm::length_t>(axis)],
			                         .furthest = cursor[static_cast<glm::length_t>(axis)]};
		}
		return false;
	}
	for (size_t axis = 0; axis < tracker.axes.size(); ++axis)
	{
		auto& swing = tracker.axes.at(axis);
		const auto at = cursor[static_cast<glm::length_t>(axis)];
		if (swing.direction == 0)
		{
			// Not swinging yet: it starts once it has gone far enough either way
			if (std::abs(at - swing.start) >= k_ShakeSwing)
			{
				swing.direction = at > swing.start ? 1 : -1;
				swing.furthest = at;
			}
			continue;
		}
		const auto direction = static_cast<float>(swing.direction);
		if ((at - swing.furthest) * direction > 0.0f)
		{
			swing.furthest = at;
		}
		else if ((swing.furthest - at) * direction >= k_ShakeSwing && std::abs(swing.furthest - swing.start) >= k_ShakeSwing)
		{
			// Turned back after a full swing
			swing.start = swing.furthest;
			swing.furthest = at;
			swing.direction = -swing.direction;
			if (tracker.turnCount < tracker.turns.size())
			{
				tracker.turns.at(tracker.turnCount++) = tracker.clock;
			}
			else
			{
				std::shift_left(tracker.turns.begin(), tracker.turns.end(), 1);
				tracker.turns.back() = tracker.clock;
			}
		}
	}
	if (tracker.turnCount == tracker.turns.size() && tracker.turns.back() - tracker.turns.front() <= k_ShakeSeconds)
	{
		tracker = {};
		return true;
	}
	return false;
}
