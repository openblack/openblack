/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Locator.h"

#define LOCATOR_IMPLEMENTATIONS

#include "ECS/Systems/Implementations/CameraBookmarkSystem.h"
#include "ECS/Systems/Implementations/DynamicsSystem.h"
#include "ECS/Systems/Implementations/LivingActionSystem.h"
#include "ECS/Systems/Implementations/PathfindingSystem.h"
#include "ECS/Systems/Implementations/RenderingSystem.h"
#include "ECS/Systems/Implementations/TownSystem.h"

using openblack::ecs::systems::CameraBookmarkSystem;
using openblack::ecs::systems::DynamicsSystem;
using openblack::ecs::systems::LivingActionSystem;
using openblack::ecs::systems::PathfindingSystem;
using openblack::ecs::systems::RenderingSystem;
using openblack::ecs::systems::TownSystem;

namespace openblack::ecs::systems
{
void InitializeGame()
{
	Locator::rendereringSystem::set<RenderingSystem>();
}

void InitializeLevel()
{
	Locator::dynamicsSystem::set<DynamicsSystem>();
	Locator::livingActionSystem::set<LivingActionSystem>();
	Locator::townSystem::set<TownSystem>();
	Locator::pathfindingSystem::set<PathfindingSystem>();
	Locator::cameraBookmarkSystem::set<CameraBookmarkSystem>();
}
} // namespace openblack::ecs::systems
