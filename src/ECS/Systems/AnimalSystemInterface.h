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

#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack::ecs::systems
{

/// The animals: the flocks of doves and bats and the packs of wolves the miracles make. A bird flock's leader flies where
/// it is sent and its followers keep a formation behind it; a wolf runs where it is sent, hunting what crosses its way.
/// The miracle's animals fade out rather than die.
class AnimalSystemInterface
{
public:
	virtual ~AnimalSystemInterface() = default;

	/// A turn of every animal
	virtual void ProcessTurn() = 0;
	/// Each frame: the animals are drawn between their last two turns, each posed by its own clip as the game's clock
	/// of the turn and the share of it gone tell
	virtual void Update(uint32_t turn, float turnFraction) = 0;
	virtual void Reset() = 0;

	/// A new flock with nobody in it yet, made at a point its leader's legs wander about
	virtual entt::entity CreateFlock(glm::vec2 centre, float domainRadius, float flockDistance) = 0;
	/// An animal a miracle makes, joining a flock, born at a random age at its size for its age (a spell wolf always
	/// grown); it faces a game angle (2048 to the circle, 0 along +x, a quarter turn along +z), and a bird flies at a
	/// height above the land
	virtual entt::entity CreateSpellAnimal(AnimalInfo type, glm::vec2 position, float heightAboveLand, uint16_t angle,
	                                       PlayerNames owner, entt::entity flock, entt::entity spell) = 0;
	/// A new size for an animal
	virtual void SetScale(entt::entity animal, float scale) = 0;
	/// An animal's radius across the ground: the larger half of its model's width and depth, scaled
	[[nodiscard]] virtual float RadiusOf(entt::entity animal) const = 0;
	/// The step an animal makes each turn, in metres across the land
	[[nodiscard]] virtual glm::vec3 MovementOf(entt::entity animal) const = 0;
	/// The point a flock's leader wanders about, moved
	virtual void SetFlockCentre(entt::entity flock, glm::vec2 centre) = 0;
	/// The flock's leader, its first animal still there, none for an empty flock
	[[nodiscard]] virtual entt::entity LeaderOf(entt::entity flock) const = 0;
	/// The flock's animals, oldest first
	[[nodiscard]] virtual std::vector<entt::entity> MembersOf(entt::entity flock) const = 0;
	/// Where an animal is heading across the land now: a wolf hunting, its prey
	[[nodiscard]] virtual glm::vec2 GoalOf(entt::entity animal) const = 0;
	/// How high above the land at its goal an animal is heading for
	[[nodiscard]] virtual float GoalHeightOf(entt::entity animal) const = 0;
	/// A bird sent on its way: to a point at a height above the land, choosing what to do next once there; the leader of
	/// its flock has the others follow it in formation once they have arrived too
	virtual void SendBird(entt::entity bird, glm::vec2 goal, float height, bool leader) = 0;
	/// A wolf sent running from where it appeared to a destination, hunting along a strip of land so wide either side
	virtual void SendWolf(entt::entity wolf, glm::vec2 start, glm::vec2 destination, float halfWidth) = 0;
	/// A miracle's animal starts to fade out, then goes
	virtual void StartFading(entt::entity animal) = 0;
	/// An effect kills an animal where it has been put down: a miracle's animal fades out, any other is left with no life
	virtual void KillByEffect(entt::entity animal, glm::vec3 position) = 0;
	/// An animal with no life left starts dying where it is: it falls dead (a bird out of the sky), then lies dead its
	/// time. One in the physics starts dying only once it has come down.
	virtual void SetDying(entt::entity animal) = 0;
	/// An animal is taken into a hand, the player's or a creature's: it plays its kind's clip for being held, if its
	/// kind has one
	virtual void IntoHand([[maybe_unused]] entt::entity animal) {}

	/// Whether an animal may take up a reaction: not held, flying or carried, and not dying, dead, brought down or holding
	/// still while a clip plays
	[[nodiscard]] virtual bool IsAvailableForReaction([[maybe_unused]] entt::entity animal) const { return false; }
	/// An animal flees from an object with no test, as it flees a fire: false only when it isn't an animal. With the
	/// object gone it gives up on its next step.
	virtual bool SetupFleeFromObject([[maybe_unused]] entt::entity animal, [[maybe_unused]] entt::entity object)
	{
		return false;
	}
	/// An animal takes up the reaction to a thing flying at a speed: it flees when the thing is nearer than it flies in
	/// two seconds, remembering it; otherwise it takes no notice. Whether it took it up.
	virtual bool SetupReactToFlyingObject([[maybe_unused]] entt::entity animal, [[maybe_unused]] entt::entity object,
	                                      [[maybe_unused]] float speed)
	{
		return false;
	}
	/// An animal's reaction ends: it forgets what it fled and decides what to do again
	virtual void StopReaction([[maybe_unused]] entt::entity animal) {}

	/// Whether a creature is frightened of an animal: bats frighten creatures, doves don't
	[[nodiscard]] virtual bool IsFrighteningToCreature(entt::entity animal) const = 0;
	/// Whether the player's hand may pick the animal up
	[[nodiscard]] virtual bool CanPlayerPickUp(entt::entity animal) const = 0;
};

} // namespace openblack::ecs::systems
