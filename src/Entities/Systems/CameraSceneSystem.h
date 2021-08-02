/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "EntitySystem.h"

#include <glm/glm.hpp>

#include <chrono>
#include <deque>

namespace openblack::entities::systems
{

class CameraSceneSystem final: public EntitySystem
{
public:
	CameraSceneSystem(bool debug) : EntitySystem(debug) {};
	void CameraPointCreated(entt::registry& registry, entt::entity entity);
	void Connect(Registry& registry) override;
	bool Update(std::chrono::duration<float, std::milli> dt) override;

private:
	std::deque<entt::entity> _startingPoints;
	entt::entity _currentPoint = entt::null;
	float _progress = 0;
	glm::vec3 _currentPosition;
};
} // namespace openblack::entities::systems
