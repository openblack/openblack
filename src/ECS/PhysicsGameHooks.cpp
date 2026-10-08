/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "PhysicsGameHooks.h"

#include "3D/MapCoords.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/DeadTree.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/Systems/ResourceStoreSystemInterface.h"
#include "ECS/VillagerMemory.h"
#include "ECS/WorldObjects.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
/// A villager starts to fly: it remembers what it was doing, unless it was in a hand, and it flies
void StartFlying(entt::entity villager)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* action = registry.TryGet<LivingAction>(villager);
	if (action == nullptr || !Locator::livingActionSystem::has_value())
	{
		return;
	}
	auto& living = Locator::livingActionSystem::value();
	const auto top = living.VillagerGetState(*action, LivingAction::Index::Top);
	if (top == VillagerStates::Flying)
	{
		return;
	}
	if (top != VillagerStates::InHand)
	{
		villager_memory::StorePreviousState(*action);
	}
	living.VillagerSetState(*action, LivingAction::Index::Top, VillagerStates::Flying, false);
}
} // namespace

PhysicsStarted PhysicsGameHooks::InitialisePhysics(DynamicsSystemInterface& dynamics, entt::entity object,
                                                   const PhysicsStart& start)
{
	const auto started = PhysicsClassHooks::InitialisePhysics(dynamics, object, start);
	if (started.started && Locator::entitiesRegistry::value().AllOf<Villager>(object))
	{
		StartFlying(object);
	}
	return started;
}

entt::entity PhysicsGameHooks::EndPhysics(DynamicsSystemInterface& dynamics, PhysicsEntry* entry, entt::entity object,
                                          bool insert)
{
	const auto kept = PhysicsClassHooks::EndPhysics(dynamics, entry, object, insert);
	auto& registry = Locator::entitiesRegistry::value();
	if (kept == entt::null || !registry.Valid(kept))
	{
		return kept;
	}
	// TODO(physics): people and animals land on their backs, fronts or feet, play their landing, and die of a fall or
	// drown in the sea; until then they stand where they came down and decide what to do
	if (auto* action = registry.TryGet<LivingAction>(kept);
	    action != nullptr && registry.AllOf<Villager>(kept) && Locator::livingActionSystem::has_value())
	{
		Locator::livingActionSystem::value().VillagerSetState(*action, LivingAction::Index::Top, VillagerStates::DecideWhatToDo,
		                                                      false);
	}
	if (auto* animal = registry.TryGet<Animal>(kept))
	{
		// It goes on from where it came down
		const auto& position = registry.Get<const Transform>(kept).position;
		animal->position = position;
		animal->previousPosition = position;
		animal->move.position = {map_coords::ToFixed(position.x), map_coords::ToFixed(position.z)};
		animal->move.goal = animal->move.position;
		animal->state = AnimalState::DecideWhatToDo;
	}
	return kept;
}

void PhysicsGameHooks::StartFlyingFromHand([[maybe_unused]] DynamicsSystemInterface& dynamics, PhysicsEntry& entry)
{
	if (Locator::entitiesRegistry::value().AllOf<Villager>(entry.entity))
	{
		StartFlying(entry.entity);
	}
}

void PhysicsGameHooks::ReactToImpact([[maybe_unused]] DynamicsSystemInterface& dynamics, PhysicsEntry& entry,
                                     const ImpactInfo& impact)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto object = entry.entity;
	const auto hitter = impact.hitBy;
	if (hitter == entt::null || !registry.Valid(hitter) || !registry.Valid(object) ||
	    !Locator::resourceStoreSystem::has_value())
	{
		return;
	}
	// A thing that is a resource and meets a store of it goes into the store: trees, dead trees and fences as wood,
	// mushrooms and animals as food, a pot or pile as what it holds, which a pile of the same also takes
	auto& stores = Locator::resourceStoreSystem::value();
	const auto resource = stores.ResourceOf(object);
	if (resource.type == ResourceType::None)
	{
		return;
	}
	// TODO(stores): a fence a script made indestructible stays out of stores; openblack has no indestructible flag yet
	if (stores.IsStore(hitter, resource.type))
	{
		stores.TakeObject(hitter, object, impact.player);
		return;
	}
	if (registry.AllOf<Pot>(object))
	{
		if (const auto* other = registry.TryGet<const Pot>(hitter);
		    other != nullptr && stores.ResourceOf(hitter).type == resource.type)
		{
			stores.AddToPile(hitter, resource.type, resource.amount);
			world_objects::LeaveGhost(object);
			world_objects::Remove(object);
		}
	}
}
