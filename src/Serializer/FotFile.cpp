#include "FotFile.h"
#include <Entities/Components/Footpath.h>
#include <spdlog/spdlog.h>

#include "3D/LandIsland.h"
#include "Common/FileSystem.h"
#include "Entities/Registry.h"
#include "Game.h"
#include "GameThingSerializer.h"

using namespace openblack;

FotFile::FotFile(Game& game)
    : _game(game)
{
}

void FotFile::Load(const fs::path& path)
{
	auto stream = _game.GetFileSystem().Open(path, FileMode::Read);
	serializer::GameThingSerializer serializer(*stream);
	auto footpathLinkSaves = serializer.DeserializeList<serializer::GameThingSerializer::FootpathLinkSave>();
	auto footpaths = serializer.DeserializeList<serializer::GameThingSerializer::Footpath>();
	auto& registry = _game.GetEntityRegistry();
	const auto& island = _game.GetLandIsland();

	// Keep track of footpath entities in order to associate them to the footpath link saves later
	std::vector<entities::components::Footpath::Id> footpathEntities;

	for (const auto& footpath : footpaths)
	{
		const auto entity = registry.Create();
		auto& footpathEntt = registry.Assign<entities::components::Footpath>(entity);
		footpathEntt.nodes.reserve(footpath._nodes.size());
		for (const auto& node : footpath._nodes)
		{
			glm::vec3 position = glm::vec3 {
			    10.0f * node._coords.x / static_cast<float>(0xFFFF),
			    node._coords.altitude,
			    10.0f * node._coords.z / static_cast<float>(0xFFFF),
			};

			// This bit is mainly for visualization, it could be that using these offsets causes uses for path planning
			// if that is the case, this bit should be moved to rendering code
			position.y += island.GetHeightAt({position.x, position.z});

			footpathEntt.nodes.push_back({position});
		}
		footpathEntities.push_back(static_cast<entities::components::Footpath::Id>(entity));
	}

	for (const auto& save : footpathLinkSaves)
	{
		std::vector<entities::components::Footpath::Id> linkFootpathEntities;
		for (const auto& footpath : save._link._footpaths)
		{
			// Save links have duplicates to entries in footpaths
			const auto footpathListItr = std::find(footpaths.cbegin(), footpaths.cend(), footpath);
			assert(footpathListItr != footpaths.cend());
			const auto footpathIndex = std::distance(footpaths.cbegin(), footpathListItr);
			linkFootpathEntities.push_back(footpathEntities[footpathIndex]);
		}
		glm::vec3 position = glm::vec3 {
		    10.0f * save._coords.x / static_cast<float>(0xFFFF),
		    save._coords.altitude,
		    10.0f * save._coords.z / static_cast<float>(0xFFFF),
		};
		const auto entity = registry.Create();
		auto& linkEntt = registry.Assign<entities::components::FootpathLink>(entity, position, std::move(linkFootpathEntities));
	}
}
