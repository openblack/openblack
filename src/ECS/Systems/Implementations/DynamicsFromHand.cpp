/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include <glm/gtx/euler_angles.hpp>
#include <spdlog/spdlog.h>

#include "DynamicsSystem.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/DeadTree.h"
#include "ECS/Components/MapCellResident.h"
#include "ECS/Components/Physics.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/PhysicsGround.h"
#include "ECS/Registry.h"
#include "ECS/Systems/FireSystemInterface.h"
#include "ECS/Systems/ReactionSystemInterface.h"
#include "Hand/HandGrabRules.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
Registry& Entities()
{
	return Locator::entitiesRegistry::value();
}

bool IsAvailable(entt::entity object)
{
	return object != entt::null && Entities().Valid(object);
}
} // namespace

// What happens as a hand, or a creature's hand, lets go of what it held. Whether it is put down where it is or flies is
// decided here, as the game decides it for every kind of thing; each kind's own part is in its hooks.

std::optional<std::pair<glm::mat3, glm::vec3>> DynamicsSystem::ReleasePose(entt::entity object, bool alignToSlope)
{
	auto body = MakeBody(object, FactsOf(object));
	const auto* land = Land();
	if (body == nullptr || land == nullptr)
	{
		return std::nullopt;
	}
	// Lifted out of the land, never pulled down to it
	body->AdjustToGroundLevel(*land, true, alignToSlope);
	const auto pose = body->ObjectPose();
	return std::pair {pose.axes, pose.origin};
}

FromHandResult DynamicsSystem::InitialisePhysicsFromHand(entt::entity object, const FromHand& release)
{
	auto& registry = Entities();
	if (!IsAvailable(object) || registry.AllOf<InPhysics>(object))
	{
		return {};
	}
	const bool byCreature = release.creature != entt::null;
	registry.AssignOrReplace<InPhysics>(object);
	// Out of the map's cells while it moves
	registry.Remove<MapCellResident, MapCellMover>(object);

	auto* entry = AddObject(
	    object, {.velocity = release.velocity, .thrower = release.creature, .player = release.player, .fromHand = !byCreature});
	if (entry == nullptr)
	{
		// A thing with no shape to move with stays where it was let go
		ObjectEndPhysics(object, true);
		return {};
	}
	auto& body = *entry->body;
	body.angularMomentum = release.angularMomentum;

	const bool anyTree = registry.AnyOf<Tree, DeadTree>(object);
	const bool thrown = hand_grab::IsThrow(release.velocity, byCreature);
	// What a creature throws flies from where it is; anything else let go is put out of the land (and onto it, laid
	// along its slope unless it is a tree, when it isn't thrown) and raised clear of what is under it
	const float before = body.Centre().y;
	float settled = before;
	if (!(byCreature && thrown))
	{
		AdjustToGroundLevel(*entry, thrown, !anyTree);
		settled = body.Centre().y;
		body.ClearForces();
		RaiseUntilNotIntersecting(*entry);
	}

	const auto* land = Land();
	const auto centre = body.Centre();
	const glm::vec2 xz(centre.x, centre.z);
	const auto facts = FactsOf(object);
	const bool living = registry.AnyOf<Villager, Animal>(object);
	const bool fence = facts.row == physics::MaterialRow::Fence;
	const bool landed =
	    land != nullptr && hand_grab::LandsOnRelease({
	                           .thrown = thrown,
	                           .raised = settled != body.Centre().y,
	                           .computerVillager = registry.AllOf<Villager>(object) && release.player.has_value() &&
	                                               *release.player != PlayerNames::PLAYER_ONE,
	                           .dryLand = land->IsDryLand(xz),
	                           .nearestAltitude = land->CellAltitudeNearest(xz),
	                           .needsGentleSlope = living || fence,
	                           .normalY = land->NormalAt(xz).y,
	                       });

	if (landed)
	{
		// TODO(physics): a villager put down by the player's own hand whose job changed plays the advisor's line for it
		entry->flags |= PhysicsEntry::k_Landed;
		Hooks().ConsiderMimickingLanding(object, release.player);
		float tiltX = 0.0f;
		float tiltY = 0.0f;
		float tiltZ = 0.0f;
		glm::extractEulerAngleYXZ(glm::mat4(registry.Get<const Transform>(object).rotation), tiltY, tiltX, tiltZ);
		const bool burning = Locator::fireSystem::has_value() && Locator::fireSystem::value().IsOnFire(object);
		switch (hand_grab::OutcomeOfLanding({.living = living,
		                                     .fence = fence,
		                                     .tree = registry.AllOf<Tree>(object),
		                                     .burning = burning,
		                                     .onLand = land->IsLand(xz),
		                                     .tiltX = tiltX,
		                                     .tiltZ = tiltZ,
		                                     .dontReplant = release.dontReplant,
		                                     .byCreature = byCreature}))
		{
		case hand_grab::LandedOutcome::LeavesPhysics:
			RemoveObject(object, true, true);
			return {.entry = nullptr, .landed = true};
		case hand_grab::LandedOutcome::Falls:
			entry->flags &= static_cast<uint8_t>(~PhysicsEntry::k_Landed);
			break;
		case hand_grab::LandedOutcome::Settles:
			break;
		}
		return {.entry = entry, .landed = true};
	}

	// It flies: a villager lets go of what it carried, people and animals near it look or run, and creatures may try to
	// catch it
	if (registry.AllOf<Villager>(object))
	{
		Hooks().DropCarriedResource(*this, object, release.velocity);
	}
	if (Locator::reactionSystem::has_value())
	{
		Locator::reactionSystem::value().Create({.initiator = object,
		                                         .type = Reaction::ReactToFlyingObject,
		                                         .player = release.player.value_or(PlayerNames::NEUTRAL),
		                                         .position = centre,
		                                         .playerless = !release.player.has_value()});
	}
	Hooks().CheckAllCreaturesForCatching(object, *entry);
	Hooks().StartFlyingFromHand(*this, *entry);
	// TODO(physics): a toy a player let go makes the player's creature think of playing with it
	return {.entry = entry, .landed = false};
}
