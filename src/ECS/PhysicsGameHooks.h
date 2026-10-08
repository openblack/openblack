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
/// thrown or knocked and stand again where they come down.
class PhysicsGameHooks: public systems::PhysicsClassHooks
{
public:
	systems::PhysicsStarted InitialisePhysics(systems::DynamicsSystemInterface& dynamics, entt::entity object,
	                                          const systems::PhysicsStart& start) override;
	entt::entity EndPhysics(systems::DynamicsSystemInterface& dynamics, PhysicsEntry* entry, entt::entity object,
	                        bool insert) override;
	void StartFlyingFromHand(systems::DynamicsSystemInterface& dynamics, PhysicsEntry& entry) override;
};

} // namespace openblack::ecs
