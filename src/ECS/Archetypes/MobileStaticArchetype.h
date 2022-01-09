/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <entt/fwd.hpp>
#include <glm/fwd.hpp>

#include "Enums.h"

namespace openblack
{
struct GMobileStaticInfo;
}

namespace openblack::ecs::archetypes
{
class MobileStaticArchetype
{
public:
	static entt::entity Create(const glm::vec3& position, MobileStaticInfo type, float altitude, float xAngleRadians,
	                           float yAngleRadians, float zAngleRadians, float scale);
	MobileStaticArchetype() = delete;
};
} // namespace openblack::ecs::archetypes
