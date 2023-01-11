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

#include "3D/LandIsland.h"
#include "3D/UnloadedIsland.h"
#include "Audio/AudioManager.h"
#include "Audio/AudioManagerNoOp.h"
#include "Common/RandomNumberManagerProduction.h"
#include "ECS/Archetypes/PlayerArchetype.h"
#include "ECS/MapProduction.h"
#include "ECS/Registry.h"
#include "ECS/Systems/Implementations/CameraBookmarkSystem.h"
#include "ECS/Systems/Implementations/DynamicsSystem.h"
#include "ECS/Systems/Implementations/LivingActionSystem.h"
#include "ECS/Systems/Implementations/PathfindingSystem.h"
#include "ECS/Systems/Implementations/PlayerSystem.h"
#include "ECS/Systems/Implementations/RenderingSystem.h"
#include "ECS/Systems/Implementations/TownSystem.h"
#if __ANDROID__
#include "FileSystem/AndroidFileSystem.h"
#else
#include "FileSystem/DefaultFileSystem.h"
#endif
#include "Resources/Resources.h"

using namespace openblack::audio;
using namespace openblack::filesystem;
using openblack::ecs::MapProduction;
using openblack::ecs::Registry;
using openblack::ecs::systems::CameraBookmarkSystem;
using openblack::ecs::systems::DynamicsSystem;
using openblack::ecs::systems::LivingActionSystem;
using openblack::ecs::systems::PathfindingSystem;
using openblack::ecs::systems::PlayerSystem;
using openblack::ecs::systems::RenderingSystem;
using openblack::ecs::systems::TownSystem;
using openblack::resources::Resources;

namespace openblack::ecs::systems
{
void InitializeGame()
{
#if __ANDROID__
	Locator::filesystem::emplace<filesystem::AndroidFileSystem>();
#else
	Locator::filesystem::emplace<filesystem::DefaultFileSystem>();
#endif
	Locator::terrainSystem::emplace<UnloadedIsland>();
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
	Locator::playerSystem::emplace<PlayerSystem>();
	Locator::rendereringSystem::emplace<RenderingSystem>();
	Locator::entitiesRegistry::emplace<Registry>();
}

void InitializeLevel(const std::filesystem::path& path)
{
	Locator::entitiesMap::emplace<MapProduction>();
	Locator::dynamicsSystem::emplace<DynamicsSystem>();
	Locator::livingActionSystem::emplace<LivingActionSystem>();
	Locator::townSystem::emplace<TownSystem>();
	Locator::pathfindingSystem::emplace<PathfindingSystem>();
	Locator::cameraBookmarkSystem::emplace<CameraBookmarkSystem>();
	Locator::terrainSystem::emplace<LandIsland>(path);
}
} // namespace openblack::ecs::systems
