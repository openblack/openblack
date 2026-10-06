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
#include <span>
#include <vector>

#include "Enums.h"

/// Who may put a leash on which creature. Each player has one creature they can lead: it is owned by the player and
/// marked leashable, and no other creature of theirs is. A player leads only that creature, so they lead at most one
/// at a time, and nobody leads another player's creature. A creature that belongs to no player can't be led at all.
namespace openblack::creature_leash
{

/// Why a player may not put a leash on a creature
enum class Refusal : uint8_t
{
	/// They may
	None,
	/// It isn't a creature
	NotACreature,
	/// The player is nobody, who leads no creature
	NoPlayer,
	/// The creature belongs to another player, or to none
	OwnedByAnother,
	/// The creature is the player's, but not the one they can lead
	NotLeashable,
	/// It hasn't learnt the learning leash, which it must before it can wear any
	DoesNotKnowLearningLeash,
	/// It hasn't learnt the leash asked for
	DoesNotKnowThatLeash,
	/// Another player holds its leash
	HeldByAnother,
};
/// A short plain-English reason, for the log and the debug windows
[[nodiscard]] const char* Describe(Refusal refusal);

/// What the rules need to know about a creature
struct Candidate
{
	PlayerNames owner {PlayerNames::NEUTRAL};
	bool leashable {false};
	bool knowsLearningLeash {false};
	/// Whether it knows the leash asked for
	bool knowsType {false};
	/// The player holding its leash, when it wears one
	std::optional<PlayerNames> heldBy;
};

/// Whether a player can lead creatures at all: every player but nobody
[[nodiscard]] bool CanLead(PlayerNames player);
/// Why the player may not put a leash on the creature, or Refusal::None when they may
[[nodiscard]] Refusal WhyNot(PlayerNames player, const Candidate& creature);

/// A creature as the one-each assignment sees it, by an id the caller picks
struct Claim
{
	uint32_t creature {0};
	PlayerNames owner {PlayerNames::NEUTRAL};
	bool leashable {false};
};

/// The creatures that stop being leashable when the chosen one, owned by the owner, is made leashable: every other one
/// of the owner's that is. The newest choice wins, as when a player is given a new creature.
[[nodiscard]] std::vector<uint32_t> Displaced(std::span<const Claim> creatures, uint32_t chosen, PlayerNames owner);
/// The player's leashable creature, if they have one
[[nodiscard]] std::optional<uint32_t> LeashableOf(std::span<const Claim> creatures, PlayerNames player);
/// Whether a new creature becomes its owner's leashable one by itself: it does when it belongs to a player and none of
/// the player's other creatures is leashable yet. A player's first creature is theirs to lead; extra ones are not.
[[nodiscard]] bool ClaimsOnArrival(std::span<const Claim> others, PlayerNames owner);

} // namespace openblack::creature_leash
