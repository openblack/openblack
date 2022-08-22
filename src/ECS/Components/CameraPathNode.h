/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <entt/entity/entity.hpp>

namespace openblack::ecs::components
{

struct CameraPathNode
{
	entt::entity next;
	entt::entity lookAt;
};

struct CameraPathStart
{
	entt::id_type from;
	uint32_t flags;
};
} // namespace openblack::ecs::components
