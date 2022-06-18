/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "PathfindingSystem.h"

#include <spdlog/spdlog.h>

#include "ECS/Components/Transform.h"
#include "ECS/Components/WallHug.h"
#include "ECS/Registry.h"
#include "Game.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

void PathfindingSystem::Update()
{
	auto& registry = Game::Instance()->GetEntityRegistry();

	// ARRIVED:
	registry.Each<const MoveStateArrivedTag, const Transform, const WallHug>(
	    [](const MoveStateArrivedTag&, const Transform&, const WallHug&) {
		    SPDLOG_LOGGER_WARN(spdlog::get("pathfinding"), "ARRIVED state not implemented in move to pathfinding");
	    });

	// LINEAR, LINEAR_CW, LINEAR_CCW
	registry.Each<const MoveStateLinearTag, const Transform, const WallHug>(
	    [](const MoveStateLinearTag&, const Transform&, const WallHug&) {
		    SPDLOG_LOGGER_WARN(spdlog::get("pathfinding"), "LINEAR* states not implemented in move to pathfinding");
	    });

	// ORBIT_CW, ORBIT_CCW
	registry.Each<const MoveStateOrbitTag, const Transform, const WallHug>(
	    [](const MoveStateOrbitTag&, const Transform&, const WallHug&) {
		    SPDLOG_LOGGER_WARN(spdlog::get("pathfinding"), "ORBIT* states not implemented in move to pathfinding");
	    });

	// STEP_THROUGH
	registry.Each<const MoveStateStepThroughTag, const Transform, const WallHug>(
	    [](const MoveStateStepThroughTag&, const Transform&, const WallHug&) {
		    SPDLOG_LOGGER_WARN(spdlog::get("pathfinding"), "STEP_THROUGH state not implemented in move to pathfinding");
	    });

	// FINAL_STEP
	registry.Each<const MoveStateFinalStepTag, const Transform, const WallHug>(
	    [](const MoveStateFinalStepTag&, const Transform&, const WallHug&) {
		    SPDLOG_LOGGER_WARN(spdlog::get("pathfinding"), "FINAL_STEP state not implemented in move to pathfinding");
	    });

	// EXIT_CIRCLE_CW, EXIT_CIRCLE_CCW:
	registry.Each<const MoveStateExitCircleTag, const Transform, const WallHug>(
	    [](const MoveStateExitCircleTag&, const Transform&, const WallHug&) {
		    SPDLOG_LOGGER_WARN(spdlog::get("pathfinding"), "EXIT_CIRCLE* states not implemented in move to pathfinding");
	    });
}
