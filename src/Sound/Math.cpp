/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Math.h"

using namespace openblack::audio;

float Math::MapTo(float val, float curMin, float curMax, float tarMin, float tarMax)
{
	return (val - curMin) / (curMax - curMin) * (tarMax - tarMin) + tarMin;
}
