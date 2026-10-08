/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Systems/DynamicsSystemInterface.h"

namespace openblack::ecs
{

/// What the game's own kinds of thing do in the physics, beyond what any object does: people and animals fly when
/// thrown or knocked, are hurt by hard knocks, and stand, drown or die where they come down; creatures are hurt by what
/// strikes them; physical shields pay for the blows they take; things that are resources go into the stores they meet;
/// trees take root again or fall dead, rocks wear and break, and handfuls spill where they come to rest.
class PhysicsGameHooks: public systems::PhysicsClassHooks
{
public:
	systems::PhysicsStarted InitialisePhysics(systems::DynamicsSystemInterface& dynamics, entt::entity object,
	                                          const systems::PhysicsStart& start) override;
	entt::entity EndPhysics(systems::DynamicsSystemInterface& dynamics, PhysicsEntry* entry, entt::entity object,
	                        bool insert) override;
	void StartFlyingFromHand(systems::DynamicsSystemInterface& dynamics, PhysicsEntry& entry) override;
	void ReactToImpact(systems::DynamicsSystemInterface& dynamics, PhysicsEntry& entry, const ImpactInfo& impact) override;
	void ImpactFeedback(systems::DynamicsSystemInterface& dynamics, PhysicsEntry& entry, bool hit) override;
	bool HasSunk(systems::DynamicsSystemInterface& dynamics, PhysicsEntry& entry) override;
	void DropSound(entt::entity object) override;
	void OfferToCatchingCreatures(entt::entity object, PhysicsEntry& entry) override;
};

} // namespace openblack::ecs
