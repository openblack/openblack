/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <cstdint>

namespace openblack::serializer
{

#pragma pack(push, 1)
struct MapCoords
{
	uint32_t x;
	uint32_t z;
	float altitude;
};
#pragma pack(pop)

} // namespace openblack::serializer
