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
#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack::ecs::components
{
struct LivingAction;
struct LivingReaction;
struct TownImpression;
} // namespace openblack::ecs::components

// What villagers do about the reactions they take up. A villager fleeing a miracle runs ten metres straight away from
// it, or across its way if it moves, at its fleeing speed; once fifty metres away from what isn't coming at it, it turns
// to watch it, and beyond a hundred it gives up. A villager watching a nice miracle turns to face it, an eighth of a
// half turn a turn. Stopping, it goes back to what it was doing.

namespace openblack::ecs::systems::villager_reactions
{

/// Whether a villager may take up a reaction: able and alive, in a state that allows it
[[nodiscard]] bool Available(entt::entity villager, Reaction type);
/// A villager takes up a reaction of a kind: it keeps what it was doing to go back to, unless it was reacting already
void Start(entt::entity villager, Reaction type, components::LivingReaction& state, bool wasReacting);
/// A villager stops reacting; when asked, it goes back to what it was doing
void Stop(entt::entity villager, components::LivingReaction& state, bool resetState);

/// The states of fleeing, and of watching (after fleeing, or a nice miracle)
uint32_t Fleeing(components::LivingAction& action);
uint32_t Watching(components::LivingAction& action);

/// The share of an impression a villager's town takes: its population for unchanged belief over its people
[[nodiscard]] float TownShare(entt::entity town);

/// When a voice of belief was last heard, and when the guidance last spoke, in game turns; the gap before the next
struct BeliefVoice
{
	bool started {false};
	uint32_t lastVoice {0};
	uint32_t lastGuidance {0};
	uint32_t gap {0};
};
/// The belief a villager gained shows as a symbol rising from it, and the awe of its people may be heard: a good or evil
/// voice, more awed the nearer the player is to leading its town, when the player isn't leading it already, now and then,
/// near enough the hand
/// A town's belief gained in a player rises as a symbol from its town centre's foot, in the player's colour
/// The villager goes at its speed for the state it is to end up in, as the game works it out at each change of state
void SetStateSpeed(entt::entity villager, VillagerStates state);
/// The town's impression, made as the town first takes one: believing in the neutral player as its tables say
components::TownImpression& ImpressionOf(entt::entity town);
void ShowTownBelief(const glm::vec3& centre, PlayerNames player, float belief);
void ShowBelief(entt::entity villager, PlayerNames player, float belief, GuidanceAlignment alignment, BeliefVoice& voice,
                uint32_t turn);

} // namespace openblack::ecs::systems::villager_reactions
