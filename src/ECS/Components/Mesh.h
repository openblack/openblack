/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "3D/AllMeshes.h"

#include <entt/entt.hpp>
#include <fmt/format.h>

namespace openblack::ecs::components
{

struct Mesh
{
	Mesh(MeshId id, int8_t submeshId, int8_t bbSubmeshId)
	    : id(entt::hashed_string(fmt::format("{}", id).c_str()))
	    , submeshId(submeshId)
	    , bbSubmeshId(bbSubmeshId)
	{
	}
	Mesh(entt::id_type id, int8_t submeshId, int8_t bbSubmeshId)
	    : id(id)
	    , submeshId(submeshId)
	    , bbSubmeshId(bbSubmeshId)
	{
	}
	entt::id_type id;
	int8_t submeshId;
	int8_t bbSubmeshId;
};

} // namespace openblack::ecs::components
