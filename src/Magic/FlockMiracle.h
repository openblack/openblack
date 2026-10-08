/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "FlockMiracleInterface.h"

namespace openblack::magic
{

/// The flock miracles in the game: their animals come from the animal system, their effects from the particle system
class FlockMiracle final: public FlockMiracleInterface
{
public:
	[[nodiscard]] ParticleType ParticleTypeOf(const ecs::components::Spell& spell) const override;
	void Start(SpellServicesInterface& services, ecs::components::Spell& spell) override;
	void ProcessTurn(SpellServicesInterface& services, ecs::components::Spell& spell) override;
	[[nodiscard]] bool AnimalsLeft(const ecs::components::Spell& spell) const override;
	[[nodiscard]] bool FollowsHand(SpellServicesInterface& services, const ecs::components::Spell& spell) const override;

private:
	/// The animals made this turn along the hand's sweep
	void Emit(SpellServicesInterface& services, ecs::components::Spell& spell);
	/// An animal that flew or ran into a shield strikes it; if the shield holds, the animal fades
	void CheckShields(SpellServicesInterface& services, ecs::components::Spell& spell);
};

} // namespace openblack::magic
