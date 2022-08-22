/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <filesystem>

#include <Debug/Camera.h>
#include <entt/entt.hpp>
#include <glm/vec3.hpp>

namespace openblack
{

std::vector<entt::entity> GetCameraInstances(entt::id_type id);

struct CameraPoint
{
	bool start;
	entt::entity next = entt::null;
	glm::vec3 position;
	glm::vec3 rotation;
};

class CameraPath
{
public:
	CameraPath(std::string debugName = "");
	virtual ~CameraPath() = default;
	/// Reads a camera path file
	bool LoadFromFile(const std::filesystem::path& path);
	[[nodiscard]] const std::vector<CameraPoint>& GetPoints() const { return _points; }
	[[nodiscard]] uint32_t GetMovementSpeed() const { return _movementSpeed; };
	/// Creates ECS entities for each camera path node. Assigns a camera-vertex component to each entity
	void CreatePathEntities(glm::vec3 position);
	/// Creates an ECS entity for the first camera path node
	void CreatePathStartEntity(glm::vec3 position);
	/// The position of the first camera path node. This is not a world position
	[[nodiscard]] glm::vec3 GetPathStartPosition() const { return _points.front().position; }
	[[nodiscard]] const std::string& GetDebugName() const { return _debugName; }

private:
	std::string _debugName;
	uint32_t _movementSpeed;
	std::vector<CameraPoint> _points;
	entt::entity _start;

protected:
	uint32_t _unk1;

	friend debug::gui::Camera;
};
} // namespace openblack