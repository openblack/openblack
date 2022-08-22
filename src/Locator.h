/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <entt/locator/locator.hpp>

namespace openblack
{

class RandomNumberManagerInterface;
class TempleInteriorInterface;

namespace resources
{
class ResourcesInterface;
}

namespace ecs::systems
{
class RenderingSystemInterface;
class DynamicsSystemInterface;
class CameraBookmarkSystemInterface;
class CameraPathSystemInterface;
class LivingActionSystemInterface;
class TownSystemInterface;
class PathfindingSystemInterface;

void InitializeGame();
void InitializeLevel();
} // namespace ecs::systems

struct Locator
{
	using resources = entt::locator<resources::ResourcesInterface>;
	using rng = entt::locator<RandomNumberManagerInterface>;
	using rendereringSystem = entt::locator<ecs::systems::RenderingSystemInterface>;
	using dynamicsSystem = entt::locator<ecs::systems::DynamicsSystemInterface>;
	using cameraBookmarkSystem = entt::locator<ecs::systems::CameraBookmarkSystemInterface>;
	using cameraPathSystem = entt::locator<ecs::systems::CameraPathSystemInterface>;
	using livingActionSystem = entt::locator<ecs::systems::LivingActionSystemInterface>;
	using townSystem = entt::locator<ecs::systems::TownSystemInterface>;
	using pathfindingSystem = entt::locator<ecs::systems::PathfindingSystemInterface>;
	using temple = entt::locator<TempleInteriorInterface>;
};
} // namespace openblack
