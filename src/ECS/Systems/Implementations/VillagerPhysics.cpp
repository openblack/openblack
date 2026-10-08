/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "VillagerPhysics.h"

#include <LNDFile.h>
#include <glm/geometric.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "3D/LandIslandInterface.h"
#include "Common/GameRandom.h"
#include "ECS/Components/HandGrab.h"
#include "ECS/Components/Indestructible.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/LivingPhysics.h"
#include "ECS/Components/LivingReaction.h"
#include "ECS/Components/Physics.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Components/VillagerDeath.h"
#include "ECS/CreatureSight.h"
#include "ECS/PhysicsEntry.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureMindSystemInterface.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/Systems/PlayerSystemInterface.h"
#include "ECS/Systems/ReactionSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "ECS/VillagerClips.h"
#include "ECS/VillagerMemory.h"
#include "ECS/WorldObjects.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Physics/Body.h"
#include "Physics/LivingRules.h"
#include "Resources/ResourcesInterface.h"
#include "VillagerFire.h"
#include "VillagerReactions.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;
using openblack::ecs::villager_clips::ClipPlayed;
using openblack::ecs::villager_clips::IsOnWater;
namespace living = openblack::physics::living;

namespace
{
Registry& Entities()
{
	return Locator::entitiesRegistry::value();
}

systems::LivingActionSystemInterface& Living()
{
	return Locator::livingActionSystem::value();
}

VillagerStates TopState(const LivingAction& action)
{
	return Living().VillagerGetState(action, LivingAction::Index::Top);
}

void SetTopState(LivingAction& action, VillagerStates state)
{
	Living().VillagerSetState(action, LivingAction::Index::Top, state, false);
}

const GVillagerInfo* InfoOf(entt::entity villager)
{
	return static_cast<const GVillagerInfo*>(world_objects::InfoOf(villager));
}

const GVillagerStateTableInfo* StateRowOf(VillagerStates state)
{
	if (!Locator::infoConstants::has_value())
	{
		return nullptr;
	}
	const auto& table = Locator::infoConstants::value().villagerStateTable;
	const auto index = static_cast<size_t>(state);
	return index < table.size() ? &table.at(index) : nullptr;
}

/// A villager stands facing a heading as the game counts it, drawn turned a quarter from its body
void Face(entt::entity villager, float heading)
{
	auto* transform = Entities().TryGet<Transform>(villager);
	if (transform == nullptr)
	{
		return;
	}
	transform->rotation = physics::QuarterTurned(living::HeadingAxes(heading));
}

bool IsDead(entt::entity villager)
{
	return Entities().AllOf<VillagerDeath>(villager);
}

/// A body that goes into the water starts dying again, lying its time without a graveyard from now
void SinkDying(entt::entity villager, LivingAction& action)
{
	if (auto* death = Entities().TryGet<VillagerDeath>(villager))
	{
		if (const auto* info = InfoOf(villager))
		{
			death->turnsLeft = info->dyingTimeWithoutGraveyard;
		}
	}
	SetTopState(action, VillagerStates::Dying);
}

/// The weight a drowning, or a death on landing in the water, has with the town against whoever caused it
constexpr float k_DrowningDeathWeight = 0.01f;

void PlayerDid(uint32_t deed, entt::entity object, PlayerNames player)
{
	if (!Locator::creatureMindSystem::has_value())
	{
		return;
	}
	const auto* transform = Entities().TryGet<const Transform>(object);
	Locator::creatureMindSystem::value().PlayerDid(deed, transform != nullptr ? transform->position : glm::vec3(0.0f), object,
	                                               player);
}
} // namespace

std::optional<PlayerNames> villager_physics::DropperOf(entt::entity object)
{
	// The hand whose last let-go thing it is: the local player's hands are the only ones on the screen
	std::optional<PlayerNames> dropper;
	Entities().Each<const HandGrab>([&dropper, object](const HandGrab& hand) {
		if (hand.lastDropped == object)
		{
			dropper =
			    Locator::playerSystem::has_value() ? Locator::playerSystem::value().GetLocalPlayer() : PlayerNames::PLAYER_ONE;
		}
	});
	return dropper;
}

bool villager_physics::StartFlying(entt::entity villager)
{
	auto& registry = Entities();
	auto* action = registry.TryGet<LivingAction>(villager);
	if (action == nullptr || !Locator::livingActionSystem::has_value())
	{
		return true;
	}
	const auto top = TopState(*action);
	if (top != VillagerStates::Flying)
	{
		if (top != VillagerStates::InHand)
		{
			villager_memory::StorePreviousState(*action);
		}
		SetTopState(*action, VillagerStates::Flying);
		// A state it can't leave keeps it out of the air
		if (TopState(*action) != VillagerStates::Flying)
		{
			return false;
		}
	}
	// It flies dead or thrown. Only a vortex bringing people from another land flings them out with the vortex clip;
	// openblack has no such vortex yet, and a tornado's passengers are simply thrown.
	registry.AssignOrReplace<VillagerClip>(
	    villager, VillagerClip {.clip = living::VillagerThrownClip(world_objects::LifeOf(villager) > 0.0f, false)});
	return true;
}

void villager_physics::IntoHand(entt::entity villager)
{
	auto* action = Entities().TryGet<LivingAction>(villager);
	if (action == nullptr || !Locator::livingActionSystem::has_value())
	{
		return;
	}
	villager_memory::StorePreviousState(*action);
	SetTopState(*action, VillagerStates::InHand);
}

void villager_physics::Land(PhysicsEntry* entry, entt::entity villager)
{
	auto& registry = Entities();
	auto* action = registry.TryGet<LivingAction>(villager);
	auto* transform = registry.TryGet<Transform>(villager);
	if (action == nullptr || transform == nullptr || !Locator::livingActionSystem::has_value())
	{
		return;
	}
	const auto player = entry != nullptr ? entry->player : std::nullopt;
	auto pose = living::LandingPose::None;
	if (entry != nullptr && entry->body != nullptr)
	{
		// How it lay at the start of its last turn: on its front, its back or its feet, facing as that leaves it
		const auto& axes = entry->body->TurnStartAxes();
		pose = living::VillagerLandingPose(axes[0].y);
		Face(villager, living::VillagerLandingHeading(pose, axes));
		// The thrower's creature, seeing it come down hurt or upright, feels angry or sorry with its player
		if (player.has_value())
		{
			const bool feet = pose == living::LandingPose::Feet;
			creature_sight::EmpathiseWithPlayer(*player, feet ? CreatureDesires::Compassion : CreatureDesires::Anger,
			                                    feet ? 0.1f : 0.5f, transform->position);
		}
	}
	// It stands on the land where it came down
	if (Locator::terrainSystem::has_value())
	{
		transform->position.y =
		    Locator::terrainSystem::value().GetHeightAt(glm::vec2(transform->position.x, transform->position.z));
	}
	const auto* info = InfoOf(villager);
	const bool alive = world_objects::LifeOf(villager) > 0.0f;
	if (IsOnWater(transform->position))
	{
		if (!alive)
		{
			if (IsDead(villager))
			{
				// A body thrown into the water sinks dying
				SinkDying(villager, *action);
			}
			else
			{
				// Killed in the air, it drowns as it lands, put down to the thrower's player
				villager_fire::DieByEffect(villager, villager_fire::DeathCause {.reason = DeathReason::PlayerInteractionDrown,
				                                                                .killer = player,
				                                                                .weight = k_DrowningDeathWeight});
			}
			return;
		}
		// It struggles in the water, its drowning put down to whoever threw it
		action->turnsUntilStateChange = info != nullptr ? info->drowningTime : 0;
		registry.AssignOrReplace<LastInteractingPlayer>(villager, LastInteractingPlayer {.player = player});
		SetTopState(*action, VillagerStates::Drowning);
		return;
	}
	if (!alive)
	{
		if (IsDead(villager))
		{
			SetTopState(*action, VillagerStates::Dead);
		}
		else
		{
			// Killed in the air, it dies of the fall where it lands, put down to the thrower's player
			villager_fire::DieByEffect(villager,
			                           villager_fire::DeathCause {.reason = DeathReason::PlayerInteraction, .killer = player});
		}
		return;
	}
	const auto previous = Living().VillagerGetState(*action, LivingAction::Index::Previous);
	SetTopState(*action, VillagerStates::Landed);
	registry.AssignOrReplace<VillagerClip>(villager, VillagerClip {.clip = living::VillagerLandedClip(pose, false)});
	// Some states it was in it goes straight back to
	// TODO(physics): a villager scripts control goes back too; openblack keeps no script control of villagers yet
	if (const auto* row = StateRowOf(previous); row != nullptr && row->field0xf4 != 0)
	{
		SetTopState(*action, previous);
	}
}

bool villager_physics::Sink(PhysicsEntry& entry)
{
	auto& registry = Entities();
	const auto villager = entry.entity;
	// Something already going from the world hasn't sunk
	auto* action = registry.Valid(villager) ? registry.TryGet<LivingAction>(villager) : nullptr;
	if (action == nullptr || !Locator::livingActionSystem::has_value())
	{
		return false;
	}
	// The player who dropped it in the sea may teach their creature to throw people in it
	if (const auto dropper = DropperOf(villager))
	{
		PlayerDid(living::k_DeedThrowInTheSea, villager, *dropper);
	}
	const auto* info = InfoOf(villager);
	if (IsDead(villager))
	{
		SinkDying(villager, *action);
		return true;
	}
	action->turnsUntilStateChange = info != nullptr ? info->drowningTime : 0;
	SetTopState(*action, VillagerStates::Drowning);
	return true;
}

bool villager_physics::TakesFlyingObjectReaction(entt::entity villager)
{
	const auto* action = Entities().TryGet<const LivingAction>(villager);
	if (action == nullptr || !Locator::livingActionSystem::has_value())
	{
		return true;
	}
	const auto top = TopState(*action);
	return top != VillagerStates::Flying && top != VillagerStates::Landed;
}

namespace
{
/// Where a flying thing is on the map now, which it is watched from
std::optional<glm::vec3> WherePositioned(entt::entity object)
{
	const auto& registry = Entities();
	const auto* transform = registry.Valid(object) ? registry.TryGet<const Transform>(object) : nullptr;
	return transform != nullptr ? std::optional(transform->position) : std::nullopt;
}

/// Whether a thing in the physics is still really in the air
bool ActuallyInTheAir(entt::entity object, const PhysicsEntry& entry)
{
	if (!Entities().AllOf<InPhysics>(object) || entry.body == nullptr)
	{
		return false;
	}
	const auto centre = entry.body->Centre();
	const float land =
	    Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt(glm::vec2(centre.x, centre.z)) : 0.0f;
	return living::IsActuallyInTheAir(entry.body->Speed(), centre.y, land, entry.body->Radius());
}
} // namespace

void villager_physics::SetupReactToFlyingObject(entt::entity villager, entt::entity object)
{
	auto& registry = Entities();
	auto* action = registry.TryGet<LivingAction>(villager);
	const auto* entry = Locator::dynamicsSystem::has_value() ? Locator::dynamicsSystem::value().Find(object) : nullptr;
	const auto there = WherePositioned(object);
	if (action == nullptr || entry == nullptr || entry->body == nullptr || !there.has_value())
	{
		return;
	}
	// Measured across the map from the villager to where the thing is, against how far it flies in two seconds
	const auto& here = registry.Get<const Transform>(villager).position;
	const float distance = living::MapDistance(here, *there);
	registry.AssignOrReplace<WatchedFlyingObject>(villager, WatchedFlyingObject {.object = object});
	if (living::RespondToFlyingObject(distance, entry->body->Speed()) == living::FlyingObjectResponse::Run)
	{
		SetTopState(*action, VillagerStates::FleeingFromObjectReaction);
		return;
	}
	SetTopState(*action, VillagerStates::PointAtFlyingObjectReaction);
	// Women and children are scared stiff one time in three; anyone else looks for it, stands or points and talks
	if (Locator::gameRandom::has_value())
	{
		auto& random = Locator::gameRandom::value();
		const auto* person = registry.TryGet<const Villager>(villager);
		const bool womanOrChild =
		    person != nullptr && (person->sex == Villager::Sex::FEMALE || person->lifeStage == Villager::LifeStage::Child);
		const auto first = womanOrChild ? random.GameRand(3) : 1;
		const auto second = first != 0 ? random.GameRand(3) : 0;
		registry.AssignOrReplace<VillagerClip>(villager,
		                                       VillagerClip {.clip = living::PointingClip(womanOrChild, first, second)});
	}
}

uint32_t villager_physics::Flying([[maybe_unused]] LivingAction& action)
{
	// The physics flies it
	return 1;
}

bool villager_physics::ExitFlying([[maybe_unused]] LivingAction& action, VillagerStates next)
{
	// Only the hand, landing, death or the water take it out of the air
	return next != VillagerStates::InHand && next != VillagerStates::Landed && next != VillagerStates::Dying &&
	       next != VillagerStates::Dead && next != VillagerStates::Drowning;
}

bool villager_physics::ExitInHand([[maybe_unused]] LivingAction& action, VillagerStates next)
{
	// Only flying, landing, death or the water take it out of the hand
	return next != VillagerStates::Flying && next != VillagerStates::Landed && next != VillagerStates::Dying &&
	       next != VillagerStates::Dead && next != VillagerStates::Drowning;
}

uint32_t villager_physics::Landed(LivingAction& action)
{
	auto& registry = Entities();
	const auto villager = registry.ToEntity(action);
	// As it starts to get up it stops calling others to it from the hand; it plays its landing once, and decides what to
	// do
	// TODO(physics): a villager put down as a disciple goes to its job; openblack has no disciples yet
	if (action.turnsSinceStateChange == 0 && Locator::reactionSystem::has_value())
	{
		Locator::reactionSystem::value().RemoveFrom(villager, Reaction::ReactToVillagerInHand);
	}
	const auto* clip = registry.TryGet<const VillagerClip>(villager);
	if (clip != nullptr && !ClipPlayed(action, clip->clip))
	{
		return 1;
	}
	SetTopState(action, VillagerStates::DecideWhatToDo);
	return 1;
}

uint32_t villager_physics::Drowning(LivingAction& action)
{
	auto& registry = Entities();
	const auto villager = registry.ToEntity(action);
	// An indestructible villager is held short of the end for ever
	const auto step = living::StepDrowning(action.turnsUntilStateChange, registry.AllOf<Indestructible>(villager));
	action.turnsUntilStateChange = step.left;
	if (step.dies)
	{
		// It drowns, put down to the player whose hand dropped it, else the last player who did something to it
		auto killer = DropperOf(villager);
		if (!killer.has_value())
		{
			if (const auto* last = registry.TryGet<const LastInteractingPlayer>(villager))
			{
				killer = last->player;
			}
		}
		villager_fire::DieByEffect(villager, villager_fire::DeathCause {.reason = DeathReason::PlayerInteractionDrown,
		                                                                .killer = killer,
		                                                                .weight = k_DrowningDeathWeight});
	}
	return 1;
}

uint32_t villager_physics::PointAtFlyingObject(LivingAction& action)
{
	auto& registry = Entities();
	const auto villager = registry.ToEntity(action);
	const auto* watched = registry.TryGet<const WatchedFlyingObject>(villager);
	const auto object = watched != nullptr ? watched->object : entt::null;
	const auto* entry =
	    object != entt::null && Locator::dynamicsSystem::has_value() ? Locator::dynamicsSystem::value().Find(object) : nullptr;
	const auto there = WherePositioned(object);
	// With the thing out of the physics, nothing changes until its reaction ends
	if (entry == nullptr || entry->body == nullptr || !there.has_value())
	{
		return 1;
	}
	// Too near, it runs; otherwise it looks at the thing, and once that is no longer really in the air it stops reacting
	const auto& here = registry.Get<const Transform>(villager).position;
	if (living::RespondToFlyingObject(living::MapDistance(here, *there), entry->body->Speed()) ==
	    living::FlyingObjectResponse::Run)
	{
		SetTopState(action, VillagerStates::FleeingFromObjectReaction);
		return 1;
	}
	systems::villager_reactions::LookAt(villager, *there);
	if (!ActuallyInTheAir(object, *entry))
	{
		registry.Remove<WatchedFlyingObject>(villager);
		if (auto* reaction = registry.TryGet<LivingReaction>(villager))
		{
			systems::villager_reactions::Stop(villager, *reaction, true);
		}
		else
		{
			SetTopState(action, VillagerStates::DecideWhatToDo);
		}
	}
	return 1;
}
