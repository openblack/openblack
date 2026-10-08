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

/// Brings the creatures' bodies to life: their shape follows what each creature has become, they are posed by their
/// animations, and their eyes look about and blink (see components::CreatureMorph, CreatureAnimation, CreatureEyes)
class CreatureAnimationSystemInterface
{
public:
	virtual ~CreatureAnimationSystemInterface() = default;

	/// Once a game turn: the fatness each body shows follows its creature's a step
	virtual void ProcessTurn() = 0;
	/// Once a frame, by the game time, which stops while the game is paused: the bodies are reshaped where they have
	/// changed enough, posed, and their eyes placed
	virtual void Update(std::chrono::duration<float, std::milli> gameTime) = 0;

	/// Where a bone of a creature is in one of its animations at a time, played left to right or not, in its mesh's space,
	/// the animation blended as the body is drawn; nothing when the creature has no such animation or bone, or hasn't been
	/// posed yet
	[[nodiscard]] virtual std::optional<glm::vec3> BoneInAnimation(entt::entity creature, size_t animation, float timeMs,
	                                                               uint32_t bone, bool mirrored) = 0;
	/// How long one of a creature's animations lasts, in milliseconds, if it has it
	[[nodiscard]] virtual std::optional<float> AnimationDuration(entt::entity creature, size_t animation) = 0;
	/// How far one of a creature's animations carries it across, in its model's units, if it has it
	[[nodiscard]] virtual std::optional<float> AnimationTravel(entt::entity creature, size_t animation) = 0;
	/// A force kicks a creature's body at a point, swaying its upper body when the point is high on it and its lower
	/// body otherwise, unless it is striking with a destroying blow
	virtual void KickSway(entt::entity creature, glm::vec3 force, glm::vec3 point) = 0;
};

} // namespace openblack::ecs::systems
