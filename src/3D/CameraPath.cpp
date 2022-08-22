/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CameraPath.h"

#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>

#include "CAMFile.h"
#include "ECS/Registry.h"
#include "FileSystem/FileSystemInterface.h"
#include "Locator.h"

using namespace openblack;

CameraPath::CameraPath(std::string debugName)
    : _debugName(std::move(debugName))
    , _start(entt::null)
{
}

void CameraPath::Load(const cam::CAMFile& file)
{
	auto filePoints = file.GetPoints();
	for (auto& filePoint : filePoints)
	{
		auto& node = _points.emplace_back();
		node.start = _points.size() == 1;
		node.next = entt::null;
		node.position = glm::make_vec3(filePoint.position.data());
		node.rotation = glm::make_vec3(filePoint.heading.data());
	}

	_duration = std::chrono::milliseconds(file.GetDuration());
}

bool CameraPath::LoadFromFile(const std::filesystem::path& path)
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading Camera Path from file: {}", path.generic_string());
	cam::CAMFile file;

	try
	{
		file.Open(Locator::filesystem::value().FindPath(path));
	}
	catch (std::runtime_error& err)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to open cam file from filesystem {}: {}", path.generic_string(),
		                    err.what());
		return false;
	}

	Load(file);
	return true;
}

bool CameraPath::LoadFromBuffer(const std::vector<uint8_t>& data)
{
	cam::CAMFile file;

	try
	{
		file.Open(data);
	}
	catch (std::runtime_error& err)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to open cam file from buffer: {}", err.what());
		return false;
	}

	Load(file);
	return true;
}

void CameraPath::CreatePathEntities(glm::vec3 position)
{
	auto& registry = Locator::entitiesRegistry::value();
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
	auto& registry = Locator::entitiesRegistry::value();
	auto firstPoint = GetPoints().front();
	firstPoint.position += position;

	auto entity = registry.Create();
	registry.Assign<CameraPoint>(entity, firstPoint);
}
