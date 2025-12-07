/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "3D/TempleInteriorInterface.h"
#include "Enums.h"

namespace openblack::ecs::components
{

struct TempleInteriorPart
{
	TempleRoom room;
};

struct Temple
{
	PlayerNames owner;
};
} // namespace openblack::ecs::components
