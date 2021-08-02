/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Common/FileSystem.h"

#include <glm/vec3.hpp>
#include <entt/entt.hpp>

namespace openblack
{

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
	CameraPath() : _start(entt::null) {};
	~CameraPath() = default;
	/// Reads a camera path file
	bool LoadFromFile(const fs::path& path);
	const std::vector<CameraPoint>& GetPoints() const { return _points; }
	/// Creates ECS entities for each camera path node. Assigns a camera-vertex component to each entity
	void CreatePathEntities(glm::vec3 position);
	/// Creates an ECS entity for the first camera path node
	void CreatePathStartEntity(glm::vec3 position);
	/// The position of the first camera path node. This is not a world position
	glm::vec3 GetPathStartPosition() const { return _points.front().position; }
private:
	std::vector<CameraPoint> _points;
	entt::entity _start;
};
}