/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "3D/AllMeshes.h"
#include "ECS/Components/DeadTree.h"
#include "ECS/Components/MagicShield.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Registry.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "ECS/WorldObjects.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Resources/ResourceManager.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::systems;

PhysicsStarted PhysicsClassHooks::InitialisePhysics(DynamicsSystemInterface& dynamics, entt::entity object,
                                                    const PhysicsStart& start)
{
	return dynamics.ObjectInitialisePhysics(object, start);
}

void PhysicsClassHooks::ReactToImpact([[maybe_unused]] DynamicsSystemInterface& dynamics, [[maybe_unused]] PhysicsEntry& entry,
                                      [[maybe_unused]] const ImpactInfo& impact)
{
}

void PhysicsClassHooks::ImpactFeedback([[maybe_unused]] DynamicsSystemInterface& dynamics, [[maybe_unused]] PhysicsEntry& entry,
                                       [[maybe_unused]] bool hit)
{
}

entt::entity PhysicsClassHooks::EndPhysics(DynamicsSystemInterface& dynamics, [[maybe_unused]] PhysicsEntry* entry,
                                           entt::entity object, bool insert)
{
	return dynamics.ObjectEndPhysics(object, insert);
}

bool PhysicsClassHooks::HasSunk([[maybe_unused]] DynamicsSystemInterface& dynamics, [[maybe_unused]] PhysicsEntry& entry)
{
	return false;
}

void PhysicsClassHooks::DropSound([[maybe_unused]] entt::entity object) {}

SoundCollisionType PhysicsClassHooks::CollideSoundType(entt::entity object) const
{
	const auto& registry = Locator::entitiesRegistry::value();
	// The wood the hand carries sounds hollow
	if (registry.AllOf<components::DeadTree>(object))
	{
		const auto* mesh = registry.TryGet<const components::Mesh>(object);
		if (mesh != nullptr && mesh->id == resources::HashIdentifier(MeshId::ObjectWoodInHand))
		{
			return SoundCollisionType::HollowWood;
		}
	}
	const auto* info = world_objects::InfoOf(object);
	return info != nullptr ? info->collideSound : SoundCollisionType::Default;
}

void PhysicsClassHooks::FelledTreeToppled([[maybe_unused]] entt::entity tree) {}

void PhysicsClassHooks::CheckAllCreaturesForCatching([[maybe_unused]] entt::entity object, [[maybe_unused]] PhysicsEntry& entry)
{
}

void PhysicsClassHooks::ForgetBuildingHitter([[maybe_unused]] entt::entity object) {}

bool PhysicsClassHooks::RaisesObjects(entt::entity object) const
{
	// A shield's dome never lifts what is dropped inside it
	return !Locator::entitiesRegistry::value().AllOf<components::MagicShield>(object);
}

std::unique_ptr<physics::Body> PhysicsClassHooks::CreatureBody([[maybe_unused]] entt::entity creature)
{
	return nullptr;
}
