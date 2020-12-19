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

#include "Enums.h"

namespace openblack::ecs::components
{

struct Abode
{
	AbodeNumber type;
	uint32_t townId;
	// If a village does not have a ABODE_STORAGE_PIT then other abodes are used
	// by the villagers
	uint32_t foodAmount;
	uint32_t woodAmount;
	/// Villager
	std::set<entt::entity> inhabitants;
};

} // namespace openblack::ecs::components
