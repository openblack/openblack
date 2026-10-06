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

#include <optional>

#include <glm/vec3.hpp>

#include "Creature/CreaturePhysiology.h"

namespace openblack::ecs::components
{

/// How a creature's body is doing: its age, energy, exhaustion, thirst, poo, warmth and life (see
/// creature_physiology). Its fatness, strength and size are the creature's own, which its body shows.
struct CreatureNeeds
{
	/// Set from the species' tables the first turn the body is looked after
	bool started {false};
	creature_physiology::Needs needs {};

	/// Whether its mind has it asleep, resting, or out cold, and for how many turns
	enum class Rest : uint8_t
	{
		Awake,
		Asleep,
		Resting,
		Unconscious,
	};
	Rest rest {Rest::Awake};
	uint32_t restTurns {0};
	/// Rested enough to wake, as of this turn
	bool rested {false};
	/// Why it should faint, as of this turn
	std::optional<creature_physiology::Faint> faint;

	/// What it carries, as heavy as itself at 1, while it carries anything
	std::optional<float> carriedWeight;
	/// Whether it was on the move this turn
	bool moving {false};
};

/// A drop of a creature's sick, flying until it lands and fades
struct CreaturePukeDrop
{
	glm::vec3 velocity {0.0f};
	float seconds {0.0f};
};

} // namespace openblack::ecs::components
