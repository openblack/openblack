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
#include "Creature/LeashRules.h"

namespace openblack::ecs::components
{

/// What a creature wants, what it is doing about it, and what it looks at
struct CreatureMindState
{
	/// The last stage of growing up
	static constexpr uint32_t k_FullyGrownUp = 13;

	/// The desires, set up from the species' tables when the mind first thinks
	std::optional<creature_desires::Desires> desires;
	creature_mind::IdleMind idle {};
	creature_look::Target look {};
	/// Whether the head turns to what it watches this turn
	bool lookingAbout {false};

	/// Seconds since the player last paid it any attention
	float secondsAlone {0.0f};
	/// Seconds since the player last stroked or slapped it, and which
	std::optional<float> feedbackSeconds;
	bool feedbackWasStroke {false};
	/// How it feels about the player, which each stroke or slap nudges, and the average of what it has been given
	float attitudeToPlayer {0.0f};
	float averageFeedback {0.0f};
	/// The last feedback the player gave it as the hand let go, from -1 (slapped) to 1 (stroked), and what it was doing
	/// then, for it to learn from
	struct Feedback
	{
		float value;
		creature_mind::Activity activity;
	};
	std::optional<Feedback> lastFeedback;

	/// How far the creature has grown up, which decides the desires it has and which of its body's needs it feels, and
	/// the stage they were last set for. Until the game's story moves it on, a creature starts fully grown up.
	uint32_t developmentPhase {k_FullyGrownUp};
	std::optional<uint32_t> desiresPhase;

	/// While paused, the mind leaves the body alone to be posed by hand
	bool paused {false};

	/// What the leash tells the mind: the desire it forces, whether the creature is following it to the hand, and what
	/// the player has shown it on the leash
	creature_leash::MindHooks leash {};
};

} // namespace openblack::ecs::components
