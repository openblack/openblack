/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

namespace openblack::audio
{

class Math
{
public:
	static float MapTo(float val, float curMin, float curMax, float tarMin, float tarMax);
};
} // namespace openblack::audio
