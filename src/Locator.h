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

namespace resources
{
class ResourcesInterface;
}

namespace ecs::systems
{
class RenderingSystemInterface;
class DynamicsSystemInterface;
class CameraBookmarkSystemInterface;
class LivingActionSystemInterface;
class TownSystemInterface;
class PathfindingSystemInterface;
} // namespace ecs::systems

struct Locator
{
	using resources = entt::service_locator<resources::ResourcesInterface>;
	using rng = entt::service_locator<RandomNumberManagerInterface>;
	using rendereringSystem = entt::service_locator<ecs::systems::RenderingSystemInterface>;
	using dynamicsSystem = entt::service_locator<ecs::systems::DynamicsSystemInterface>;
	using cameraBookmarkSystem = entt::service_locator<ecs::systems::CameraBookmarkSystemInterface>;
	using livingActionSystem = entt::service_locator<ecs::systems::LivingActionSystemInterface>;
	using townSystem = entt::service_locator<ecs::systems::TownSystemInterface>;
	using pathfindingSystem = entt::service_locator<ecs::systems::PathfindingSystemInterface>;
};
} // namespace openblack
