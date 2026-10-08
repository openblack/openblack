/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

#include <chrono>
#include <optional>

#include <entt/entity/fwd.hpp>
#include <glm/vec3.hpp>

namespace openblack::ecs::systems
{

/// What the creatures do with the things about them (see components::CreatureObjectAction): they walk up to things and
/// pick them up, hold them in their hands, look them over, eat them, put them down, toss them away or throw them, knock
/// them down, and point at them. What is let go of flies until it comes to rest. The creatures' towns watch what they
/// do with fear or respect. Minds and the debug tools tell the creatures what to do.
class CreatureObjectActionSystemInterface
{
public:
	/// How the creature's last action went
	enum class State : uint8_t
	{
		/// It hasn't been told to do anything
		Idle,
		Busy,
		Done,
		Failed,
	};

	virtual ~CreatureObjectActionSystemInterface() = default;

	/// Once a game turn: creatures walk up to what they act on, what they carry makes them stronger, and their towns'
	/// views of them change
	virtual void ProcessTurn() = 0;
	/// Once a frame before the bodies are posed, by the game time: the actions play on
	virtual void Update(std::chrono::duration<float, std::milli> gameTime) = 0;
	/// Once a frame after the bodies are posed: things are taken hold of and let go of, what is held rides in the hand,
	/// and what was let go of flies
	virtual void LateUpdate(std::chrono::duration<float, std::milli> gameTime) = 0;

	/// Each returns whether the creature could start. Picking up and knocking down need empty hands, all those that act
	/// on what it holds need something held.
	virtual bool PickUp(entt::entity creature, entt::entity object) = 0;
	virtual bool PutDown(entt::entity creature) = 0;
	virtual bool Discard(entt::entity creature) = 0;
	virtual bool Lob(entt::entity creature) = 0;
	virtual bool EatHeld(entt::entity creature) = 0;
	/// Strokes, shakes, smells or examines what it holds, by the animation of that
	virtual bool Keep(entt::entity creature, size_t animation) = 0;
	virtual bool Throw(entt::entity creature, const glm::vec3& target) = 0;
	virtual bool Destroy(entt::entity creature, entt::entity target) = 0;
	virtual bool PointAt(entt::entity creature, const glm::vec3& point) = 0;
	/// Catches something flying at it, when it can still reach where it passes
	virtual bool Catch(entt::entity creature, entt::entity object) = 0;
	/// Stops what it is doing, keeping hold of whatever it holds
	virtual void Cancel(entt::entity creature) = 0;
	/// Lets go of what it holds at once, dropping it from the hand
	virtual void Drop(entt::entity creature) = 0;

	[[nodiscard]] virtual State GetState(entt::entity creature) const = 0;
	/// How far through its animations the action is, 0 to 1, while it plays them
	[[nodiscard]] virtual std::optional<float> GetProgress(entt::entity creature) const = 0;
	[[nodiscard]] virtual std::optional<entt::entity> GetHeld(entt::entity creature) const = 0;
	/// What something is worth to eat, if anything
	[[nodiscard]] virtual std::optional<float> FoodValueOf(entt::entity object) const = 0;
	/// Whether a creature can pick something up: things and villagers, not the land's fixtures nor other creatures
	[[nodiscard]] virtual bool CanPickUp(entt::entity object) const = 0;
	/// Whether a creature can knock something down: trees, homes and things
	[[nodiscard]] virtual bool CanDestroy(entt::entity target) const = 0;
};

} // namespace openblack::ecs::systems
