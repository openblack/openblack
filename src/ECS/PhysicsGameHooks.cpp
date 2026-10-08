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
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/WorldObjects.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
/// A villager starts to fly: what it was doing is kept, unless it was in a hand, and it flies
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
		living.VillagerSetState(*action, LivingAction::Index::Previous, top, true);
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
