/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <set>
#include <string>
#include <unordered_map>

namespace openblack::ecs::components
{

struct Town
{
	uint32_t id;
	std::unordered_map<std::string, float> beliefs;
	bool uninhabitable = false;
	std::set<entt::entity> homelessVillagers;
};

} // namespace openblack::ecs::components
