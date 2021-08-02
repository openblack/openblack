/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Creature.h"
#include "CreatureBody.h"

namespace openblack::entities::components
{

struct Creature
{
	std::string owner;
	CreatureBody::Species species;
	std::string mindName;
};
}