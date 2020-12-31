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
	auto footpath_link_saves = serializer.DeserializeList<serializer::GameThingSerializer::FootpathLinkSave>();
	auto footpaths = serializer.DeserializeList<serializer::GameThingSerializer::Footpath>();
	auto& registry = _game.GetEntityRegistry();
	const auto& island = _game.GetLandIsland();

	for (const auto& footpath : footpaths)
	{
		const auto entity = registry.Create();
		auto& footpath_entt = registry.Assign<entities::components::Footpath>(entity);
		footpath_entt.nodes.reserve(footpath._nodes.size());
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

			footpath_entt.nodes.push_back({position});
		}
	}
}
