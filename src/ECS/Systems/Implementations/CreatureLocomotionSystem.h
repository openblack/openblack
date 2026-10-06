/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>
#include <random>

#include "Creature/CreatureRoute.h"
#include "ECS/Systems/CreatureLocomotionSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::components
{
struct CreatureLocomotion;
} // namespace openblack::ecs::components

namespace openblack::ecs::systems
{

class CreatureLocomotionSystem final: public CreatureLocomotionSystemInterface
{
public:
	void ProcessTurn() override;
	void Update(float turnFraction) override;

	MoveResult MoveTo(entt::entity creature, glm::vec2 point, Pace pace, float minDistance, float maxDistance) override;
	MoveResult LeadTo(entt::entity creature, glm::vec2 point, float pull, float maxDistance) override;
	MoveResult MoveToObject(entt::entity creature, entt::entity target, Pace pace, float extra) override;
	MoveResult Follow(entt::entity creature, entt::entity target, float distance, Pace pace) override;
	MoveResult FleeFrom(entt::entity creature, glm::vec2 threat) override;
	bool TurnToFace(entt::entity creature, glm::vec2 point) override;
	void Stop(entt::entity creature) override;
	[[nodiscard]] bool IsMoving(entt::entity creature) const override;
	[[nodiscard]] const creature_route::WalkableLand& GetWalkableLand() override;

private:
	/// Starts a move for a fraction of top speed, the creature's state already gathered
	MoveResult StartMove(entt::entity creature, components::CreatureLocomotion& locomotion, glm::vec2 point, float fraction,
	                     float minDistance, float maxDistance);

	/// Where creatures can walk, sorted from the land the first time it is wanted. The system is made anew with each
	/// land.
	std::optional<creature_route::WalkableLand> _land;
	/// Fidgets and running away distances are chosen at random, apart from the game's own random numbers
	std::mt19937 _random {std::random_device {}()};
};

} // namespace openblack::ecs::systems
