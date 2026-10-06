/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <string_view>

#include <entt/entity/fwd.hpp>

namespace openblack::ecs::systems
{

/// The creatures' bodies (see components::CreatureNeeds): once a game turn they age, grow, get hungry, tired, thirsty,
/// warm or cold, heal and rest while asleep, and may faint; their fatness, strength and size follow, which their
/// bodies show. Minds tell it what the creature ate, drank and did. Its own time can be sped up to watch a creature
/// grow and get hungry.
class CreaturePhysiologySystemInterface
{
public:
	virtual ~CreaturePhysiologySystemInterface() = default;

	/// Once a game turn
	virtual void ProcessTurn() = 0;
	/// Once a frame: drops of sick fly and fall
	virtual void Update(float seconds) = 0;

	/// The creature ate something of a food value
	virtual void Eat(entt::entity creature, float foodValue) = 0;
	/// Drank its fill
	virtual void Drink(entt::entity creature) = 0;
	/// Had a poo, which drops behind it
	virtual void Poo(entt::entity creature) = 0;
	/// Was sick, spraying drops of it in front
	virtual void Puke(entt::entity creature) = 0;
	/// Came round from a faint
	virtual void WakeFromFaint(entt::entity creature) = 0;
	/// Finished an action, by its name in the game's creature action table: it costs energy and tires it, and hard
	/// work makes it stronger
	virtual void FinishAction(entt::entity creature, std::string_view action) = 0;
	/// Grew stronger, or weaker, by some amount
	virtual void ModifyStrength(entt::entity creature, float amount) = 0;

	/// How many game turns of the body pass each game turn
	virtual void SetTimeScale(float scale) = 0;
	[[nodiscard]] virtual float GetTimeScale() const = 0;
	/// Whether creatures faint when exhausted, starved or out of life
	virtual void SetFaintingEnabled(bool enabled) = 0;
	[[nodiscard]] virtual bool IsFaintingEnabled() const = 0;
};

} // namespace openblack::ecs::systems
