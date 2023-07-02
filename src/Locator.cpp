/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Locator.h"

#define LOCATOR_IMPLEMENTATIONS

#include <spdlog/spdlog.h>

#include "Audio/AudioManager.h"
#include "Audio/AudioManagerNoOp.h"
#include "Common/RandomNumberManagerProduction.h"
#include "ECS/Systems/Implementations/CameraBookmarkSystem.h"
#include "ECS/Systems/Implementations/DynamicsSystem.h"
#include "ECS/Systems/Implementations/LivingActionSystem.h"
#include "ECS/Systems/Implementations/PathfindingSystem.h"
#include "ECS/Systems/Implementations/RenderingSystem.h"
#include "ECS/Systems/Implementations/TownSystem.h"
#include "Resources/Resources.h"

using namespace openblack::audio;
using openblack::ecs::systems::CameraBookmarkSystem;
using openblack::ecs::systems::DynamicsSystem;
using openblack::ecs::systems::LivingActionSystem;
using openblack::ecs::systems::PathfindingSystem;
using openblack::ecs::systems::RenderingSystem;
using openblack::ecs::systems::TownSystem;
using openblack::resources::Resources;

namespace openblack::ecs::systems
{
void InitializeGame()
{
	Locator::resources::emplace<Resources>();
	Locator::rng::emplace<RandomNumberManagerProduction>();
	try
	{
		Locator::audio::emplace<AudioManager>();
	}
	catch (std::runtime_error& error)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("audio"), "Falling back to no-op audio: {}", error.what());
		Locator::audio::emplace<AudioManagerNoOp>();
	}
	Locator::rendereringSystem::emplace<RenderingSystem>();
}

void InitializeLevel()
{
	Locator::dynamicsSystem::emplace<DynamicsSystem>();
	Locator::livingActionSystem::emplace<LivingActionSystem>();
	Locator::townSystem::emplace<TownSystem>();
	Locator::pathfindingSystem::emplace<PathfindingSystem>();
	Locator::cameraBookmarkSystem::emplace<CameraBookmarkSystem>();
}
} // namespace openblack::ecs::systems
