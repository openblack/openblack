/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "CameraPath.h"

#include <glm/gtc/type_ptr.hpp>

#include "CAMFile.h"
#include "ECS/Registry.h"
#include "Game.h"

using namespace openblack;

CameraPath::CameraPath(std::string debugName)
    : _debugName(std::move(debugName))
    , _start(entt::null)
{
}

bool CameraPath::LoadFromFile(const std::filesystem::path& path)
{
	cam::CAMFile file;
	file.Open(path.string());
	auto filePoints = file.GetPoints();

	for (auto& filePoint : filePoints)
	{
		auto& node = _points.emplace_back();
		node.start = _points.size() == 1;
		node.position = glm::make_vec3(filePoint.position.data());
		node.rotation = glm::make_vec3(filePoint.heading.data());
	}

	_movementSpeed = file.GetMovementSpeed();
	return true;
}

void CameraPath::CreatePathEntities(glm::vec3 position)
{
	auto& registry = Game::Instance()->GetEntityRegistry();
	auto points = GetPoints();
	auto entities = std::vector<entt::entity>(points.size());
	registry.Create(entities.begin(), entities.end());
	// Add a null entity for the last point offset
	entities.emplace_back(entt::null);

	for (size_t i = 0; i < points.size(); i++)
	{
		points[i].next = entities[i + 1];
		points[i].position += position;
		registry.Assign<CameraPoint>(entities[i], points[i]);
	}
}

void CameraPath::CreatePathStartEntity(glm::vec3 position)
{
	auto& registry = Game::Instance()->GetEntityRegistry();
	auto firstPoint = GetPoints().front();
	firstPoint.position += position;

	auto entity = registry.Create();
	registry.Assign<CameraPoint>(entity, firstPoint);
}
