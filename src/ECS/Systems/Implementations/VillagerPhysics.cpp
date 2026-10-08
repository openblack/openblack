/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "VillagerPhysics.h"

#include <chrono>

#include <LNDFile.h>
#include <glm/geometric.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "3D/LandIslandInterface.h"
#include "Common/GameRandom.h"
#include "ECS/Components/CarriedByTornado.h"
#include "ECS/Components/HandGrab.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/LivingPhysics.h"
#include "ECS/Components/LivingReaction.h"
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

/// A clip's length in milliseconds, none for one not loaded
std::optional<float> ClipMilliseconds(AnimId clip)
{
	if (!Locator::resources::has_value())
	{
		return std::nullopt;
	}
	auto& animations = Locator::resources::value().GetAnimations();
	const auto key = static_cast<entt::id_type>(clip);
	if (!animations.Contains(key))
	{
		return std::nullopt;
	}
	return static_cast<float>(animations.Handle(key)->GetDuration());
}

/// Whether the clip has played through once since the villager went into its state
bool ClipPlayed(const LivingAction& action, AnimId clip)
{
	constexpr auto k_TurnMilliseconds = std::chrono::milliseconds(ecs::systems::TimeSystemInterface::k_TurnDuration).count();
	return static_cast<float>(action.turnsSinceStateChange) * static_cast<float>(k_TurnMilliseconds) >=
	       ClipMilliseconds(clip).value_or(0.0f);
}

/// Whether the land under a point is water, the shallow shore included
bool IsOnWater(glm::vec3 point)
{
	if (!Locator::terrainSystem::has_value() || point.x < 0.0f || point.z < 0.0f)
	{
		return false;
	}
	const auto* cell = Locator::terrainSystem::value().FindCell(glm::u16vec2(glm::floor(glm::vec2(point.x, point.z) / 10.0f)));
	return cell != nullptr && cell->properties.hasWater != 0;
}

/// A villager stands facing a heading as the game counts it, drawn turned a quarter from its body
void Face(entt::entity villager, float heading)
{
	auto* transform = Entities().TryGet<Transform>(villager);
	if (transform == nullptr)
	{
		return;
	}
	const glm::mat3 body(glm::vec3(std::cos(heading), 0.0f, std::sin(heading)), glm::vec3(0.0f, 1.0f, 0.0f),
	                     glm::vec3(-std::sin(heading), 0.0f, std::cos(heading)));
	transform->rotation = physics::QuarterTurned(body);
}

bool IsDead(entt::entity villager)
{
	return Entities().AllOf<VillagerDeath>(villager);
}

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

void villager_physics::StartFlying(entt::entity villager)
{
	auto& registry = Entities();
	auto* action = registry.TryGet<LivingAction>(villager);
	if (action == nullptr || !Locator::livingActionSystem::has_value())
	{
		return;
	}
	const auto top = TopState(*action);
	if (top == VillagerStates::Flying)
	{
		return;
	}
	if (top != VillagerStates::InHand)
	{
		villager_memory::StorePreviousState(*action);
	}
	SetTopState(*action, VillagerStates::Flying);
	// It flies dead, carried round a vortex, or thrown
	registry.AssignOrReplace<VillagerClip>(
	    villager, VillagerClip {.clip = living::VillagerThrownClip(world_objects::LifeOf(villager) > 0.0f,
	                                                               registry.AllOf<CarriedByTornado>(villager))});
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
	registry.AssignOrReplace<LivingLanding>(villager, LivingLanding {.pose = pose});
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
				SetTopState(*action, VillagerStates::Dying);
			}
			else
			{
				// TODO(physics): the death is put down to the thrower's player once deaths keep who caused them
				villager_fire::DieByEffect(villager);
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
			// Killed in the air, it dies of the fall where it lands
			// TODO(physics): the death is put down to the thrower's player once deaths keep who caused them
			villager_fire::DieByEffect(villager);
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
	auto* action = registry.TryGet<LivingAction>(villager);
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
		SetTopState(*action, VillagerStates::Dying);
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

void villager_physics::SetupReactToFlyingObject(entt::entity villager, entt::entity object)
{
	auto& registry = Entities();
	auto* action = registry.TryGet<LivingAction>(villager);
	const auto* entry = Locator::dynamicsSystem::has_value() ? Locator::dynamicsSystem::value().Find(object) : nullptr;
	if (action == nullptr || entry == nullptr || entry->body == nullptr)
	{
		return;
	}
	const auto& here = registry.Get<const Transform>(villager).position;
	const float distance = glm::distance(here, entry->body->Centre());
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

uint32_t villager_physics::Landed(LivingAction& action)
{
	auto& registry = Entities();
	const auto villager = registry.ToEntity(action);
	// It stops calling others to it from the hand, plays its landing once, and decides what to do
	// TODO(physics): a villager put down as a disciple goes to its job; openblack has no disciples yet
	if (Locator::reactionSystem::has_value())
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
	// TODO(physics): an indestructible villager never drowns; openblack has no indestructible flag yet
	const auto step = living::StepDrowning(action.turnsUntilStateChange, false);
	action.turnsUntilStateChange = step.left;
	if (step.dies)
	{
		// TODO(physics): the drowning is put down to the dropper's player, else the last to interact, once deaths keep
		// who caused them
		villager_fire::DieByEffect(villager);
	}
	return 1;
}

uint32_t villager_physics::PointAtFlyingObject(LivingAction& action)
{
	auto& registry = Entities();
	const auto villager = registry.ToEntity(action);
	const auto* watched = registry.TryGet<const WatchedFlyingObject>(villager);
	const auto* entry = watched != nullptr && Locator::dynamicsSystem::has_value()
	                        ? Locator::dynamicsSystem::value().Find(watched->object)
	                        : nullptr;
	// Once it is no longer in the air, the villager stops reacting
	if (entry == nullptr || !entry->IsFlying())
	{
		if (auto* reaction = registry.TryGet<LivingReaction>(villager))
		{
			systems::villager_reactions::Stop(villager, *reaction, true);
		}
		else
		{
			SetTopState(action, VillagerStates::DecideWhatToDo);
		}
		return 1;
	}
	const auto& here = registry.Get<const Transform>(villager).position;
	if (living::RespondToFlyingObject(glm::distance(here, entry->body->Centre()), entry->body->Speed()) ==
	    living::FlyingObjectResponse::Run)
	{
		SetTopState(action, VillagerStates::FleeingFromObjectReaction);
	}
	return 1;
}
