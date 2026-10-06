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
#include <glm/vec2.hpp>

namespace openblack::creature_route
{
class WalkableLand;
} // namespace openblack::creature_route

namespace openblack::ecs::systems
{

/// Gets the creatures about the land: they plan routes round what is in their way, walk and run along them, turn and
/// step off, and are drawn moving smoothly between game turns (see components::CreatureLocomotion). Minds and the
/// debug tools tell them where to go.
class CreatureLocomotionSystemInterface
{
public:
	/// What became of a request to move
	enum class MoveResult : uint8_t
	{
		/// Nowhere a creature can stand
		InvalidDestination,
		/// The creature can't walk: it has no walk or run animations, or isn't a creature
		Busy,
		Started,
	};
	enum class Pace : uint8_t
	{
		Walk,
		Run,
	};

	virtual ~CreatureLocomotionSystemInterface() = default;

	/// Once a game turn: routes are planned and the creatures move
	virtual void ProcessTurn() = 0;
	/// Once a frame: the creatures are placed and their legs posed between the last two turns, by how far through the
	/// turn the frame is, 0 to 1
	virtual void Update(float turnFraction) = 0;

	/// Walks or runs to anywhere from minDistance to maxDistance away from a point
	virtual MoveResult MoveTo(entt::entity creature, glm::vec2 point, Pace pace, float minDistance, float maxDistance) = 0;
	/// Walks or runs up to something, stopping some way short of it by both their sizes, and up to extra further
	virtual MoveResult MoveToObject(entt::entity creature, entt::entity target, Pace pace, float extra) = 0;
	/// Keeps within a distance of something, following it as it moves, until stopped
	virtual MoveResult Follow(entt::entity creature, entt::entity target, float distance, Pace pace) = 0;
	/// Runs away from a point
	virtual MoveResult FleeFrom(entt::entity creature, glm::vec2 threat) = 0;
	/// Turns on the spot to face a point
	virtual bool TurnToFace(entt::entity creature, glm::vec2 point) = 0;
	/// Stops where it is
	virtual void Stop(entt::entity creature) = 0;
	/// Whether it is planning a route, turning or on its way
	[[nodiscard]] virtual bool IsMoving(entt::entity creature) const = 0;

	/// Where the creatures can walk on this land
	[[nodiscard]] virtual const creature_route::WalkableLand& GetWalkableLand() = 0;
};

} // namespace openblack::ecs::systems
