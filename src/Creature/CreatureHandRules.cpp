/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureHandRules.h"

using namespace openblack;

bool creature_hand::MayHold(const Holdable& creature)
{
	// The game also keeps the hand off a creature part way into one particular action of its own; that action isn't
	// known here
	return creature.owner != PlayerNames::NEUTRAL && creature.species != CreatureType::Ogre && !creature.asleep &&
	       !creature.frozen;
}

bool creature_hand::ShowsInteractTip(PlayerNames player, PlayerNames owner, bool asleep)
{
	return player != PlayerNames::NEUTRAL && owner == player && !asleep;
}

bool creature_hand::IsClick(float heldMs, bool strokedOrSlapped)
{
	return heldMs < k_ClickMaxMs && !strokedOrSlapped;
}
