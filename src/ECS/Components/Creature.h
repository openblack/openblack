/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "3D/CreatureBody.h"
#include "Creature.h"
#include "Enums.h"

namespace openblack::ecs::components
{

struct Creature
{
	std::string owner;
	CreatureType species;
	std::string mindName;
};
} // namespace openblack::ecs::components