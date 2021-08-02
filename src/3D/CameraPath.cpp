/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "CameraPath.h"
#include "CamFile.h"
#include "Game.h"
#include "Entities/Registry.h"

using namespace openblack;

bool CameraPath::LoadFromFile(const fs::path& path)
{
	cam::CamFile file;
	file.Open(path.string());
	auto filePoints = file.GetPoints();

	for (auto& filePoint : filePoints)
	{
		auto& node = _points.emplace_back();
		node.start = _points.size() == 1;
		node.position = filePoint.position;
		node.rotation = glm::normalize(node.position + glm::vec3(filePoint.pitch, filePoint.yaw, filePoint.roll));
	}

	return true;
}

void CameraPath::CreatePathEntities(glm::vec3 position)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	auto points = GetPoints();
	auto entities = std::vector<entt::entity>(points.size());
	registry.Create(entities.begin(), entities.end());
	// Add a null entity for the last point offset
	entities.emplace_back(entt::null);

	for (auto i = 0; i < points.size(); i++)
	{
		points[i].next = entities[i + 1];
		points[i].position += position;
		registry.Assign<CameraPoint>(entities[i], points[i]);
	}
}

void CameraPath::CreatePathStartEntity(glm::vec3 position)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	auto firstPoint = GetPoints().front();
	firstPoint.position += position;

	auto entity = registry.Create();
	registry.Assign<CameraPoint>(entity, firstPoint);
}
