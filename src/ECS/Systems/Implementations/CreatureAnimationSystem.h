/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <random>

#include "ECS/Systems/CreatureAnimationSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class CreatureAnimationSystem final: public CreatureAnimationSystemInterface
{
public:
	void ProcessTurn() override;
	void Update(std::chrono::duration<float, std::milli> gameTime) override;
	[[nodiscard]] std::optional<glm::vec3> BoneInAnimation(entt::entity creature, size_t animation, float timeMs, uint32_t bone,
	                                                       bool mirrored) override;
	[[nodiscard]] std::optional<float> AnimationDuration(entt::entity creature, size_t animation) override;
	[[nodiscard]] std::optional<float> AnimationTravel(entt::entity creature, size_t animation) override;
	void KickSway(entt::entity creature, glm::vec3 force, glm::vec3 point) override;

private:
	/// The eyes blink at random, apart from the game's own random numbers
	std::mt19937 _random {std::random_device {}()};
};

} // namespace openblack::ecs::systems
