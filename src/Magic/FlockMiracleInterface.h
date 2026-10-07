/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "Enums.h"

namespace openblack::ecs::components
{
struct Spell;
}

namespace openblack::magic
{
class SpellServicesInterface;

/// What the flock miracles do beyond their particles: the flying flock makes doves or bats and the ground flock wolves
/// along the hand's sweep after the cast, sends them off, and fades them out when it is over
class FlockMiracleInterface
{
public:
	FlockMiracleInterface() = default;
	FlockMiracleInterface(const FlockMiracleInterface&) = delete;
	FlockMiracleInterface& operator=(const FlockMiracleInterface&) = delete;
	FlockMiracleInterface(FlockMiracleInterface&&) = delete;
	FlockMiracleInterface& operator=(FlockMiracleInterface&&) = delete;
	virtual ~FlockMiracleInterface() = default;

	/// The miracle's own particle effect: the doves' sparkles or the bats' smoke by the caster's alignment, the wolves'
	/// dust
	[[nodiscard]] virtual ParticleType ParticleTypeOf(const ecs::components::Spell& spell) const = 0;
	/// It starts: its flock, and its sweep from the hand
	virtual void Start(SpellServicesInterface& services, ecs::components::Spell& spell) = 0;
	/// Its turn: the animals the sweep makes, shields they fly into, and their fading once it is over
	virtual void ProcessTurn(SpellServicesInterface& services, ecs::components::Spell& spell) = 0;
	/// Whether it still has animals about, which keep it alive
	[[nodiscard]] virtual bool AnimalsLeft(const ecs::components::Spell& spell) const = 0;
	/// Whether it still follows its caster's hand: while it has animals still to make
	[[nodiscard]] virtual bool FollowsHand(SpellServicesInterface& services, const ecs::components::Spell& spell) const = 0;
};

} // namespace openblack::magic
