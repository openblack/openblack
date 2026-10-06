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

#include <entt/entity/fwd.hpp>

#include "Creature/CreatureMarks.h"
#include "Creature/CreatureTattoo.h"

namespace openblack::ecs::systems
{

/// Paints the creatures' skins (see components::CreatureSkin): blended towards evil or good as each body is drawn,
/// with its tattoos, wounds and blood over them, painted again whenever one of those changes. Heals the marks as the
/// game's turns go by.
class CreatureSkinSystemInterface
{
public:
	virtual ~CreatureSkinSystemInterface() = default;

	/// Once a frame, after the bodies are brought up to date: paints the skins that have changed, a new creature's
	/// straight away
	virtual void Update() = 0;
	/// Once a game turn: the marks heal a little
	virtual void ProcessTurn() = 0;

	/// A tattoo put in one of a creature's slots, or an empty slot to take one off
	virtual void SetTattoo(entt::entity creature, size_t slot, const creature_tattoo::Slot& tattoo) = 0;
	/// A wound or burn, or a drop of blood, on a creature's skin
	virtual void AddWound(entt::entity creature, const creature_marks::Mark& wound) = 0;
	virtual void AddBlood(entt::entity creature, const creature_marks::Mark& blood) = 0;
	/// A creature's marks healed by some counts at once, as a heal effect does
	virtual void Heal(entt::entity creature, uint32_t counts) = 0;
};

} // namespace openblack::ecs::systems
