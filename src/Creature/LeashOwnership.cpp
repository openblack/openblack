/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "LeashOwnership.h"

#include <algorithm>

using namespace openblack;
using namespace openblack::creature_leash;

const char* creature_leash::Describe(Refusal refusal)
{
	switch (refusal)
	{
	case Refusal::None:
		return "allowed";
	case Refusal::NotACreature:
		return "it isn't a creature";
	case Refusal::NoPlayer:
		return "nobody can lead a creature";
	case Refusal::OwnedByAnother:
		return "the creature belongs to another player";
	case Refusal::NotLeashable:
		return "the creature isn't the one this player can lead";
	case Refusal::DoesNotKnowLearningLeash:
		return "the creature hasn't learnt the learning leash";
	case Refusal::DoesNotKnowThatLeash:
		return "the creature hasn't learnt that leash";
	case Refusal::HeldByAnother:
		return "another player holds its leash";
	}
	return "unknown";
}

bool creature_leash::CanLead(PlayerNames player)
{
	return player != PlayerNames::NEUTRAL && player < PlayerNames::_COUNT;
}

Refusal creature_leash::WhyNot(PlayerNames player, const Candidate& creature)
{
	if (!CanLead(player))
	{
		return Refusal::NoPlayer;
	}
	if (creature.owner != player)
	{
		return Refusal::OwnedByAnother;
	}
	if (!creature.leashable)
	{
		return Refusal::NotLeashable;
	}
	if (creature.heldBy.has_value() && *creature.heldBy != player)
	{
		return Refusal::HeldByAnother;
	}
	if (!creature.knowsLearningLeash)
	{
		return Refusal::DoesNotKnowLearningLeash;
	}
	if (!creature.knowsType)
	{
		return Refusal::DoesNotKnowThatLeash;
	}
	return Refusal::None;
}

std::vector<uint32_t> creature_leash::Displaced(std::span<const Claim> creatures, uint32_t chosen, PlayerNames owner)
{
	std::vector<uint32_t> displaced;
	for (const auto& claim : creatures)
	{
		if (claim.creature != chosen && claim.owner == owner && claim.leashable)
		{
			displaced.push_back(claim.creature);
		}
	}
	return displaced;
}

std::optional<uint32_t> creature_leash::LeashableOf(std::span<const Claim> creatures, PlayerNames player)
{
	const auto found =
	    std::ranges::find_if(creatures, [player](const Claim& claim) { return claim.owner == player && claim.leashable; });
	return found != creatures.end() ? std::optional(found->creature) : std::nullopt;
}

bool creature_leash::ClaimsOnArrival(std::span<const Claim> others, PlayerNames owner)
{
	return CanLead(owner) && !LeashableOf(others, owner).has_value();
}
