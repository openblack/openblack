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
#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include "Creature/CreatureFight.h"

namespace openblack::ecs::components
{

/// A creature in a fight, from walking to its place in the arena until it has finished showing off or walked off. Its
/// mind leaves it alone meanwhile.
struct CreatureFighting
{
	enum class Stage : uint8_t
	{
		/// Walking to its place in the arena
		Approach,
		/// Turning to face the opponent and taunting it
		Taunt,
		/// Waiting for the opponent to be ready
		Ready,
		Duel,
		/// Having won: finishing, then showing off
		Celebrate,
		/// Having won and being evil: walking up to the loser, then having a poo on it
		PooOnLoser,
		/// After a fight that ended otherwise: finishing, then happy or sad
		Respond,
	};
	Stage stage {Stage::Approach};
	entt::entity opponent {entt::null};
	creature_fight::Arena arena {};
	/// Whether this creature made the arena, taking its place to the east of the middle
	bool madeArena {false};
	creature_fight::Fighter fighter {};
	/// How far each of its blows reaches and where it lands, measured as the fight starts, and when in each blow it
	/// lands, by the blow's place from the first
	std::vector<creature_fight::Reach> reaches;
	std::array<float, creature_fight::animations::k_AttackCount> hitTimesMs {};
	bool measured {false};
	/// How far the action's own movement had carried it, as a share of the action
	float movedShare {0.0f};
	/// Seconds in the stage, and until it next shows its anger in its face
	float stageSeconds {0.0f};
	float faceSeconds {0.0f};
	/// Whether it has been told what to do in this stage yet, and has taunted its opponent
	bool played {false};
	bool taunted {false};
	/// Whether its life has paid for the fight and its mind learnt from it
	bool ended {false};
	/// Where it stood as the fight started, the home it is taken to when it has no other
	glm::vec3 startPosition {0.0f};
};

/// What a creature has learnt of fighting, kept between fights
struct CreatureFightRecord
{
	/// How it leans in fights, from -1 (defensive) to 1 (aggressive), which the temple's belts show
	float tendency {0.0f};
	bool foughtBefore {false};
	uint32_t fights {0};
	uint32_t wins {0};
	/// Seconds since its last fight ended
	float secondsSinceFight {creature_fight::k_SecondsBetweenFights};
};

/// A creature knocked out: lying where it fell, then taken home, resting there until better, and getting up again
struct CreatureKnockedOut
{
	enum class Stage : uint8_t
	{
		Lying,
		/// Fading out where it lies and back in at home
		FadingOut,
		FadingIn,
		/// Lying at home a few seconds
		Waiting,
		/// Lying until the spells on it have worn off
		WaitingForSpells,
		Resting,
		GettingUp,
	};
	Stage stage {Stage::Lying};
	float seconds {0.0f};
	/// Decided as it faints: whether it rests to get better before it gets up
	bool rest {false};
	/// Only a script kills a creature for good: it never gets up
	bool permanent {false};
	/// Where it is taken, if it is taken home
	std::optional<glm::vec3> home;
};

} // namespace openblack::ecs::components
