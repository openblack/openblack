/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "SpellLifetime.h"

using namespace openblack::magic;

SpellFate openblack::magic::FateOf(const SpellLife& life)
{
	if (life.keptByKind || life.effectRunning)
	{
		return SpellFate::Continue;
	}
	if (life.hasParticleType)
	{
		// Its effect has gone, closed down or not
		return SpellFate::Delete;
	}
	return life.closedDown ? SpellFate::Delete : SpellFate::Continue;
}

SeedAfterCast openblack::magic::SeedAfterCastOf(bool keptInHand, bool deletedOnceCast, bool followsSpell)
{
	// The creature spells' phials are both kept and deleted: deleted wins
	if (deletedOnceCast)
	{
		return SeedAfterCast::Deleted;
	}
	if (keptInHand)
	{
		return SeedAfterCast::StaysInHand;
	}
	return followsSpell ? SeedAfterCast::FollowsSpell : SeedAfterCast::Deleted;
}
