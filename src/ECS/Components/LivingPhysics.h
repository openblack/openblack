/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>

#include <entt/entity/entity.hpp>

#include "3D/AllMeshes.h"
#include "Enums.h"
#include "Physics/LivingRules.h"

namespace openblack::ecs::components
{

/// How a villager or animal came down from its last flight, which its landing clip follows
struct LivingLanding
{
	physics::living::LandingPose pose {physics::living::LandingPose::None};
};

/// The clip a villager's state chose for it, where the state picks its own rather than its table's
struct VillagerClip
{
	AnimId clip {AnimId::Invalid};
};

/// The player who last did something to a villager, whom its drowning is put down to when no hand dropped it
struct LastInteractingPlayer
{
	std::optional<PlayerNames> player;
};

/// The flying thing a villager is pointing at
struct WatchedFlyingObject
{
	entt::entity object {entt::null};
};

} // namespace openblack::ecs::components
