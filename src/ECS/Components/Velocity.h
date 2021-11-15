/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

namespace openblack::ecs::components
{

struct Velocity
{
	float dX;
	float dY;
	float dZ;
};

} // namespace openblack::ecs::components
