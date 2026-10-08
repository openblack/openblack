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
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Physics.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/PhysicsClasses.h"
#include "ECS/PhysicsGround.h"
#include "ECS/Registry.h"
#include "ECS/Systems/FireSystemInterface.h"
#include "ECS/Systems/PlayerSystemInterface.h"
#include "ECS/Systems/ReactionSystemInterface.h"
#include "Hand/HandGrabRules.h"
#include "InfoConstants.h"
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

/// The model of a static, from its row of the tables; none for anything else
std::optional<MeshId> StaticModel(entt::entity object)
{
	const auto* still = Entities().TryGet<const MobileStatic>(object);
	if (still == nullptr || !Locator::infoConstants::has_value())
	{
		return std::nullopt;
	}
	const auto& rows = Locator::infoConstants::value().mobileStatic;
	const auto index = static_cast<size_t>(still->type);
	return index < rows.size() ? std::optional(rows[index].meshId) : std::nullopt;
}
} // namespace

void DynamicsSystem::ConsiderToyPlay(entt::entity object, const FromHand& release)
{
	if (!release.player.has_value() || release.creature != entt::null || !IsAvailable(object))
	{
		return;
	}
	if (const auto model = StaticModel(object); model.has_value() && physics_classes::IsToyModel(*model))
	{
		Hooks().ConsiderMimickingToyPlay(object, *release.player);
	}
}

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
	body->SettleOnLand(*land, true, alignToSlope);
	const auto pose = body->ObjectPose();
	return std::pair {pose.axes, pose.origin};
}

FromHandResult DynamicsSystem::LetGoFromHand(entt::entity object, const FromHand& release)
{
	auto& registry = Entities();
	if (!IsAvailable(object) || registry.AllOf<InPhysics>(object))
	{
		return {};
	}
	const bool byCreature = release.creature != entt::null;
	LetGoOfLeashesTiedTo(object);
	registry.AssignOrReplace<InPhysics>(object);
	// Out of the map's cells while it moves
	registry.Remove<MapCellResident, MapCellMover>(object);

	auto* entry = AddObject(
	    object, {.velocity = release.velocity, .thrower = release.creature, .player = release.player, .fromHand = !byCreature});
	if (entry == nullptr)
	{
		// A thing with no shape to move with is left as the game leaves it: counted in the physics, out of the map
		return {.accepted = true};
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
		SettleOnLand(*entry, thrown, !anyTree);
		settled = body.Centre().y;
		body.ClearForces();
		RaiseClearOfWhatIsUnder(*entry);
	}

	const auto* land = Land();
	const auto centre = body.Centre();
	const glm::vec2 xz(centre.x, centre.z);
	const bool living = registry.AnyOf<Villager, Animal>(object);
	const auto model = StaticModel(object);
	const bool fence = model.has_value() && physics_classes::IsFenceModel(*model);
	const bool landed =
	    land != nullptr && hand_grab::LandsOnRelease({
	                           .thrown = thrown,
	                           .raised = settled != body.Centre().y,
	                           .computerVillager = registry.AllOf<Villager>(object) && release.player.has_value() &&
	                                               Locator::playerSystem::has_value() &&
	                                               Locator::playerSystem::value().IsComputerPlayer(*release.player),
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
			ConsiderToyPlay(object, release);
			return {.accepted = true, .entry = nullptr, .landed = true};
		case hand_grab::LandedOutcome::Falls:
			entry->flags &= static_cast<uint8_t>(~PhysicsEntry::k_Landed);
			break;
		case hand_grab::LandedOutcome::Settles:
			break;
		}
		ConsiderToyPlay(object, release);
		return {.accepted = true, .entry = entry, .landed = true};
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
	Hooks().OfferToCatchingCreatures(object, *entry);
	Hooks().StartFlyingFromHand(*this, *entry);
	ConsiderToyPlay(object, release);
	return {.accepted = true, .entry = entry, .landed = false};
}
