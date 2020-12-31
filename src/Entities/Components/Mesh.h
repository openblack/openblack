/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Common/MeshLookup.h"

namespace openblack::entities::components
{

struct Mesh
{
	MeshId id;
	int8_t submeshId;
	int8_t bbSubmeshId;
};

} // namespace openblack::entities::components
