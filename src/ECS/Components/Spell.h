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

#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"
#include "Magic/SpellChants.h"
#include "Magic/SpellRules.h"
#include "Particles/ParticleSpellLink.h"

namespace openblack::ecs::components
{

/// Who casts a miracle and pays for it
struct SpellCaster
{
	enum class Kind : uint8_t
	{
		/// Gone: the miracle closes down
		None,
		/// A player, by hand or, for the neutral player, by script
		Player,
		/// A creature (its casting comes later)
		Creature,
		/// Any other object, which gives it all the prayer power it asks for
		Object,
	};
	Kind kind {Kind::None};
	PlayerNames player {PlayerNames::NEUTRAL};
	/// The creature or object, none for a player
	entt::entity entity {entt::null};
	/// A player's miracle cast from a seed made without an icon of their worship (a seed from a globe or a dispenser when
	/// they have no icon for it): the player made that seed themself, and tops up its miracle with nothing
	bool withoutIcon {false};
};

/// A running miracle. Positions are points of the world.
struct Spell
{
	MagicType magicType {MagicType::None};
	magic::SpellClass spellClass {magic::SpellClass::General};
	SpellCaster caster;
	magic::SpellChants chants;
	/// Seconds it has run, and how long it may, negative for until its prayer power runs out
	float age {0.0f};
	float duration {-1.0f};
	/// How big it was cast: a shield's radius, a fireball's size
	float magnitude {magic::k_DefaultSpellMagnitude};
	/// Where it acts now: where it was cast, then where its last event happened
	glm::vec3 position {0.0f};
	/// Where it was cast, following the hand while it is held there
	glm::vec3 castPosition {0.0f};
	glm::vec3 originalCastPosition {0.0f};
	/// The way it was cast, and the movement of its last event
	glm::vec3 direction {0.0f};
	glm::vec3 movement {1.0f, 0.0f, 0.0f};
	/// What it hands its particle effect each turn
	particles::ProcessInfo processInfo;
	/// Its particle effect, 0 for none
	uint32_t effect {0};
	/// A further effect at the point it was cast that plays out by itself, such as a storm's swirl, 0 for none
	uint32_t castEffect {0};
	/// The seed in the hand that cast it, if any
	entt::entity seed {entt::null};
	/// The object it was cast on, if any: a creature spell goes once its creature has
	entt::entity target {entt::null};
	bool closedDown {false};
	/// Its magic type starts a particle effect: it lives while that does
	bool hasParticleType {false};
	/// Cast by this computer's player's hand
	bool fromLocalHand {false};
	/// Held in this computer's hand: a locked miracle (lightning, water, food, wood) follows the hand
	bool castFromHand {false};
	/// The reaction the living near it are having to it, 0 for none yet
	uint32_t reaction {0};
	bool humanCasting {false};
	/// How many objects it may still make, -1 for no limit
	int maxObjectsToCreate {-1};

	// What some kinds of miracle keep
	/// Food and wood: the first grain has landed
	bool resourceFirstDone {false};
	/// Water: its age when its last drop left a ring on the land
	float lastRipple {0.0f};
	/// The forest: it has planted its trees, and how many of them still stand
	bool forestPlanted {false};
	uint32_t objectCount {0};
};

} // namespace openblack::ecs::components
