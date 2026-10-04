/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <set>
#include <string>
#include <unordered_map>

#include <entt/fwd.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack::ecs::components
{

struct Town
{
	uint32_t id;
	std::unordered_map<std::string, float> beliefs;
	bool uninhabitable = false;

	glm::vec3 cachedCongregationPosition;
	std::set<entt::entity> homelessVillagers;

	const glm::vec3& GetCongregationPos();
};

} // namespace openblack::ecs::components
