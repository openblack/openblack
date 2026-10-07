/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

namespace openblack::ecs::components
{

/// A tree that belongs to one of the land's forests, by the forest's number in the land's script
struct ForestMember
{
	uint32_t forest {0};
};

} // namespace openblack::ecs::components
