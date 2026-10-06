/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <chrono>

#include "Enums.h"

namespace openblack
{
struct GMagicEffectInfo;
struct GMagicInfo;
} // namespace openblack

// The prayer power ("chants") a running miracle holds: how strong that makes it, what it pays each turn and for each
// event, and how its caster tops it up. A miracle starts with a store of prayer power; it is at full strength while the
// store is above its safety level and weakens in proportion below it. Pure functions of the miracle's store, its rules
// and the caster behind it.

namespace openblack::magic
{
/// The safety level of a miracle that is not maintained covers this many seconds of its upkeep
inline constexpr float k_ChantSafetySeconds = 5.0f;

/// The length of a game turn, which turns an upkeep per turn into an upkeep per second
inline constexpr std::chrono::milliseconds k_ChantTurnDuration {100};

/// The prayer power a running miracle holds
struct SpellChants
{
	float chants {0.0f};             ///< what is left
	float initialChants {0.0f};      ///< what it held when last filled
	float strengthMultiplier {1.0f}; ///< scales the strength the prayer power gives
	bool free {false};               ///< pays nothing and always has full strength
};

/// Why prayer power was drawn from the caster
enum class ChantCharge : uint8_t
{
	PerTurn,
	PerEvent,
};

/// The caster behind a miracle: the player, a worship site's spell icon, a creature or another object. It gives the
/// miracle more prayer power when the miracle asks for it.
class SpellCasterInterface
{
public:
	virtual ~SpellCasterInterface() = default;

	/// The miracle asks for this much prayer power; returns what the caster gives
	virtual float MaintainSpell(float amount) = 0;

	/// The miracle spent prayer power. Worship sites draw it as a path of light from the site to the miracle.
	virtual void OnChantsSpent([[maybe_unused]] float chants, [[maybe_unused]] ChantCharge charge) {}
};

/// What the chant rules read besides the miracle's own store
struct SpellChantRules
{
	float costPerEvent {0.0f};             ///< what each event (a bolt striking, a fireball landing) costs
	float costToMaintain {0.0f};           ///< what one turn of upkeep costs now
	bool maintained {false};               ///< see IsMaintainedSpell
	bool recharged {false};                ///< whether the caster tops the miracle up
	bool divideCostsByTribalPower {false}; ///< whether a strong tribal power makes it cheaper
	float tribalPower {1.0f};              ///< the caster's tribal power for this miracle
	float seedPower {1.0f};                ///< the power of the seed that cast it, 1 without one
	std::chrono::milliseconds turnDuration {k_ChantTurnDuration};
};

/// The rules of a magic type from its tables, with the plain upkeep of its effect record. Miracles whose upkeep
/// depends on their size (the shields) override costToMaintain.
[[nodiscard]] SpellChantRules ChantRulesFor(MagicType type, const GMagicInfo& magic, const GMagicEffectInfo& effect);

/// Fills the miracle's store
void SetChants(SpellChants& spell, float chants);

/// The prayer power under which the miracle weakens: all of a maintained miracle's store; otherwise
/// k_ChantSafetySeconds of upkeep, at most the store it was filled with and at least the cost of one event
[[nodiscard]] float GetChantSafetyLevel(const SpellChants& spell, const SpellChantRules& rules);

/// The miracle's strength: its store over its safety level, between 0 and 1, times the tribal power, the seed's power
/// and the miracle's multiplier. 0 once the caster is gone.
[[nodiscard]] float GetSpellStrength(const SpellChants& spell, const SpellChantRules& rules,
                                     const SpellCasterInterface* caster);

/// How much of the shortfall under the safety level a payment asks the caster for
enum class Refill : uint8_t
{
	UpToCost, ///< at most what was just paid
	Whole,    ///< all of it
};

/// Takes a cost from the store, then asks a recharging caster to refill the shortfall under the safety level. Returns
/// the new strength: 0 once the caster is gone, 1 for a free miracle.
float PayFor(SpellChants& spell, const SpellChantRules& rules, SpellCasterInterface* caster, float cost,
             Refill refill = Refill::UpToCost);

/// One turn of upkeep; full strength without paying when the miracle has no upkeep
float PayForOneTurn(SpellChants& spell, const SpellChantRules& rules, SpellCasterInterface* caster);

/// The cost of one event
float PayForOneEvent(SpellChants& spell, const SpellChantRules& rules, SpellCasterInterface* caster);

/// A recharging caster tops the store up to the safety level; returns what it gave
float Recharge(SpellChants& spell, const SpellChantRules& rules, SpellCasterInterface& caster);
} // namespace openblack::magic
