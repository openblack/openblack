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

#include <entt/entity/entity.hpp>

#include "Enums.h"
#include "Magic/MagicTables.h"
#include "Magic/PrayerRules.h"

namespace openblack::ecs::components
{

/// A miracle held in the player's hand, ready to cast. Its model is drawn in the hand for the seeds that show one; the
/// others are only the miracle's in-hand effect.
struct SpellSeed
{
	SpellSeedType seedType {SpellSeedType::None};
	/// The power-up level it casts at, magic::k_BasePowerUpLevel for its plain miracle
	int powerUp {magic::k_BasePowerUpLevel};
	PlayerNames player {PlayerNames::PLAYER_ONE};
	/// Its charge of prayer power, which a seed from a dispenser is given in full
	float chantStore {0.0f};
	/// A creature has learnt from a miracle it cast: no creature learns from its miracles again
	bool learnedFrom {false};
	/// What its last miracle had left when the hand let go, to carry on with; negative for none
	float storedChants {-1.0f};
	float storedAge {0.0f};
	int storedMaxObjects {-1};
	/// Scales the prayer power and the time of what it casts
	float castMultiplier {1.0f};
	/// Multiplies the strength of what it casts
	float power {1.0f};
	/// Summoned from the player's worship, or taken from a bubble
	magic::SeedOrigin origin {magic::SeedOrigin::Bubble};
	/// Made at an icon of the player's worship: summoned from it, or taken from a globe or a dispenser while the player has
	/// an icon for the seed. Only such a seed can be powered up; its worship tops up what it casts, and it gives back
	/// what it holds when dropped.
	bool hasIcon {false};
	/// Held long enough to cast (a seed from a dispenser is ready at once)
	bool ready {false};
	/// How the hand holds it: as a miracle not yet ready until it is ready, then as its record says. The hand looks again
	/// each game turn.
	HoldType holdType {HoldType::Magic};
	/// Out of the hand, bound to the miracle it cast, and gone with it: the storms', the shields' and the forest's
	bool followsSpell {false};
	uint32_t turnsInHand {0};
	/// The miracle it cast that still runs from it, if any
	entt::entity spell {entt::null};
	/// Its miracle has been cast at least once
	bool hasCast {false};
	/// Its miracle's in-hand effect, 0 for none
	uint32_t handEffect {0};
};

} // namespace openblack::ecs::components
