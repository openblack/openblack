/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "Enums.h"

namespace openblack::ecs::components
{

struct Mobile
{
	char dummy;
};

struct MobileStatic
{
	MobileStaticInfo type; ///< This is 32 bits but could be 8 bits if stored in uint8_t
};

struct MobileObject
{
	MobileObjectInfo type; ///< This is 32 bits but could be 8 bits if stored in uint8_t
};

} // namespace openblack::ecs::components
