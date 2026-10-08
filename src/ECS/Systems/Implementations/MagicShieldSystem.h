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

#include <unordered_map>
#include <vector>

#include <glm/vec3.hpp>

#include "ECS/Systems/MagicShieldSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::components
{
struct MagicShield;
struct Spell;
} // namespace openblack::ecs::components

namespace openblack::ecs::systems
{

class MagicShieldSystem final: public MagicShieldSystemInterface
{
public:
	void ProcessTurn() override;
	void Update(float seconds) override;
	void Reset() override;
	void ShieldStruck(entt::entity spell, bool destroyed) override;
	[[nodiscard]] bool HasObject(entt::entity spell) const override;
	[[nodiscard]] std::vector<Avoid> CreatureAvoids(PlayerNames creaturePlayer, bool scripted) const override;
	[[nodiscard]] bool KeepsReactionOff(glm::vec3 watcher, glm::vec3 initiator) const override;
	[[nodiscard]] std::optional<entt::entity> ShieldAt(glm::vec3 point) const override;
	[[nodiscard]] std::vector<DomeDraw> GetDomes(float fraction) const override;

private:
	/// Raises the object of a shield miracle that has none yet
	void Raise(entt::entity spell, const components::Spell& miracle);
	/// The shield's miracle has let go of it: the spiritual shield's goes at once, the dome starts to fade
	void LetGo(entt::entity object, components::MagicShield& shield);
	/// Takes away what goes with a shield: its rings against influence and the reactions to it
	void Strip(entt::entity object, components::MagicShield& shield);
	/// A turn of a physical shield's dome; false once it has gone
	bool StepDome(entt::entity object);
	/// A reaction to a shield the villagers take up: the shield's object, its kind and reach, the turn the first villager
	/// took it up (or it was last struck), and those taking it up now
	struct ShieldReaction
	{
		entt::entity object {entt::null};
		Reaction type {Reaction::None};
		float radius {0.0f};
		uint32_t firstReacted {0};
		std::vector<entt::entity> reacting;
	};
	/// A reaction to a shield is made: it is spread to the villagers about it at once, out to its table's reach
	uint32_t CreateReaction(entt::entity object, Reaction type, glm::vec3 position, float multiplier);
	/// A reaction goes; the villagers taking it up stop, going back to what they did when asked
	void RemoveReaction(uint32_t id, bool setState);
	void RemoveReactions(entt::entity object, Reaction type);
	/// One reaction of all the land's is spread each turn in turn: its villagers in reach may take it up
	void ProcessReactions();
	void SpreadReaction(uint32_t id);
	void ApplyToVillager(entt::entity villager, uint32_t id);
	void StartReacting(entt::entity villager, uint32_t id);
	void StopReacting(entt::entity villager, bool setState);
	/// Each villager's reaction lasts as long as its kind says
	void ProcessVillagers();
	[[nodiscard]] uint8_t PriorityFor(entt::entity villager, const ShieldReaction& reaction, float distance) const;
	/// How long before a villager reacts to the same kind again, and how long it keeps reacting: the table's turns,
	/// whatever its distance, but for the shield's own reaction
	[[nodiscard]] uint32_t AgainTurnsFor(entt::entity villager, const ShieldReaction& reaction) const;
	[[nodiscard]] uint32_t ReactTurnsFor(entt::entity villager, const ShieldReaction& reaction) const;
	/// Whether something in a physical shield's dome is cut off from a reaction outside it
	[[nodiscard]] bool Blocked(glm::vec3 living, glm::vec3 source) const;
	/// A thrown thing struck a dome
	void Impact(entt::entity object, entt::entity hitter, float momentum, std::optional<PlayerNames> player) override;

	/// Each shield miracle's object
	std::unordered_map<entt::entity, entt::entity> _objects;
	std::unordered_map<uint32_t, ShieldReaction> _reactions;
	/// The next of the land's reactions to spread
	size_t _cursor {0};
};

} // namespace openblack::ecs::systems
