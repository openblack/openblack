/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "FotFile.h"

#include <spdlog/spdlog.h>

#include "3D/LandIslandInterface.h"
#include "ECS/Components/Footpath.h"
#include "ECS/Registry.h"
#include "FileSystem/FileSystemInterface.h"
#include "Game.h"
#include "GameThingSerializer.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::filesystem;

FotFile::FotFile(Game& game)
    : _game(game)
{
}

void FotFile::Load(const std::filesystem::path& path)
{
	auto stream = Locator::filesystem::value().Open(path, Stream::Mode::Read);
	serializer::GameThingSerializer serializer(*stream);
	auto footpathLinkSaves = serializer.DeserializeList<serializer::GameThingSerializer::FootpathLinkSave>();
	auto footpaths = serializer.DeserializeList<serializer::GameThingSerializer::Footpath>();
	auto& registry = _game.GetEntityRegistry();
	const auto& island = Locator::terrainSystem::value();

	// Keep track of footpath entities in order to associate them to the footpath link saves later
	std::vector<ecs::components::Footpath::Id> footpathEntities;

	for (const auto& footpath : footpaths)
	{
		const auto entity = registry.Create();
		auto& footpathEntt = registry.Assign<ecs::components::Footpath>(entity);
		footpathEntt.nodes.reserve(footpath.nodes.size());
		for (const auto& node : footpath.nodes)
		{
			glm::vec3 position = glm::vec3 {
			    10.0f * node.coords.x / static_cast<float>(0xFFFF),
			    node.coords.altitude,
			    10.0f * node.coords.z / static_cast<float>(0xFFFF),
			};

			// This bit is mainly for visualization, it could be that using these offsets causes uses for path planning
			// if that is the case, this bit should be moved to rendering code
			position.y += island.GetHeightAt({position.x, position.z});

			footpathEntt.nodes.push_back({position});
		}
		footpathEntities.push_back(static_cast<ecs::components::Footpath::Id>(entity));
	}

	for (const auto& save : footpathLinkSaves)
	{
		std::vector<ecs::components::Footpath::Id> linkFootpathEntities;
		for (const auto& footpath : save.link.footpaths)
		{
			// Save links have duplicates to entries in footpaths
			const auto footpathListItr = std::find(footpaths.cbegin(), footpaths.cend(), footpath);
			assert(footpathListItr != footpaths.cend());
			const auto footpathIndex = std::distance(footpaths.cbegin(), footpathListItr);
			linkFootpathEntities.push_back(footpathEntities[footpathIndex]);
		}
		glm::vec3 position = glm::vec3 {
		    10.0f * save.coords.x / static_cast<float>(0xFFFF),
		    save.coords.altitude,
		    10.0f * save.coords.z / static_cast<float>(0xFFFF),
		};
		const auto entity = registry.Create();
		registry.Assign<ecs::components::FootpathLink>(entity, position, std::move(linkFootpathEntities));
	}
}
