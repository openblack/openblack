/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CameraPath.h"

#include <algorithm>

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

CameraPath::Sample CameraPath::SampleAt(std::chrono::milliseconds time) const
{
	if (_points.empty())
	{
		return {};
	}
	const auto pointCount = static_cast<uint32_t>(_points.size());
	const auto durationMs = static_cast<uint32_t>(_duration.count());
	if (pointCount < 2 || durationMs == 0)
	{
		return {_points.front().position, _points.front().focus};
	}
	auto timeMs = static_cast<uint32_t>(std::max<int64_t>(time.count(), 0));
	if (timeMs > durationMs)
	{
		timeMs = durationMs - 1;
	}
	auto index = static_cast<uint32_t>(static_cast<uint64_t>(pointCount - 1) * timeMs / durationMs);
	if (index >= pointCount)
	{
		index %= pointCount;
	}
	const auto next = index + 1 < pointCount ? index + 1 : index;

	// The fraction of the way to the next point, as InnerCamera works it out, with whole milliseconds of a point's span
	const float span = static_cast<float>(durationMs) / static_cast<float>(pointCount - 1);
	const auto spansDone = static_cast<int32_t>(static_cast<float>(timeMs) / span);
	const auto spanMs = static_cast<int32_t>(span);
	const float fraction = static_cast<float>(static_cast<int32_t>(timeMs) - spansDone * spanMs) / span;

	const auto& from = _points[index];
	const auto& to = _points[next];
	return {from.position * (1.0f - fraction) + to.position * fraction, from.focus * (1.0f - fraction) + to.focus * fraction};
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
		node.focus = glm::make_vec3(filePoint.focus.data());
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
