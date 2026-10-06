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

#include "Creature/CreatureDesires.h"
#include "Creature/CreatureIdleMind.h"
#include "Creature/CreatureLook.h"

namespace openblack::ecs::components
{

/// What a creature wants, what it is doing about it, and what it looks at
struct CreatureMindState
{
	/// The desires, set up from the species' tables when the mind first thinks
	std::optional<creature_desires::Desires> desires;
	creature_mind::IdleMind idle {};
	creature_look::Target look {};
	/// Whether the head turns to what it watches this turn
	bool lookingAbout {false};

	/// What the body is like, for the desires that grow from it. Energy runs down and exhaustion builds up at made-up
	/// rates until the creature eats, sleeps and tires itself out for real.
	float energy {1.0f};
	float exhaustion {0.0f};
	/// Seconds since the player last paid it any attention
	float secondsAlone {0.0f};
	/// Seconds since the player last stroked or slapped it, and which
	std::optional<float> feedbackSeconds;
	bool feedbackWasStroke {false};

	/// How far the creature has grown up, which decides the desires it has, and the stage they were last set for
	uint32_t developmentPhase {0};
	std::optional<uint32_t> desiresPhase;

	/// While paused, the mind leaves the body alone to be posed by hand
	bool paused {false};
};

} // namespace openblack::ecs::components
