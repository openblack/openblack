/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <entt/core/hashed_string.hpp>

#include "3D/AllMeshes.h"

template <>
struct fmt::formatter<openblack::MeshId>: fmt::formatter<uint32_t>
{
	auto format(openblack::MeshId c, format_context& ctx) { return formatter<uint32_t>::format(static_cast<uint32_t>(c), ctx); }
};

namespace openblack::resources
{

inline entt::id_type MeshIdToResourceId(openblack::MeshId id)
{
	return entt::hashed_string(fmt::format("{}", id).c_str());
}

} // namespace openblack::resources
