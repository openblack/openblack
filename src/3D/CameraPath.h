/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <entt/fwd.hpp>
#include <glm/vec3.hpp>

namespace openblack
{
namespace cam
{
class CAMFile;
}

std::vector<entt::entity> GetCameraInstances(entt::id_type id);

struct CameraPoint
{
	bool start;
	entt::entity next;
	glm::vec3 position;
	glm::vec3 rotation;
};

class CameraPath
{
public:
	explicit CameraPath(std::string debugName = "");
	virtual ~CameraPath() = default;

	void Load(const cam::CAMFile& file);
	bool LoadFromFile(const std::filesystem::path& path);
	bool LoadFromBuffer(const std::vector<uint8_t>& data);

	[[nodiscard]] const std::vector<CameraPoint>& GetPoints() const { return _points; }
	[[nodiscard]] std::chrono::milliseconds GetDuration() const { return _duration; };
	/// Creates ECS entities for each camera path node. Assigns a camera-vertex component to each entity
	void CreatePathEntities(glm::vec3 position);
	/// Creates an ECS entity for the first camera path node
	void CreatePathStartEntity(glm::vec3 position);
	/// The position of the first camera path node. This is not a world position
	[[nodiscard]] glm::vec3 GetPathStartPosition() const { return _points.front().position; }
	[[nodiscard]] const std::string& GetDebugName() const { return _debugName; }

private:
	std::string _debugName;
	std::chrono::milliseconds _duration;
	std::vector<CameraPoint> _points;
	entt::entity _start;
};
} // namespace openblack
