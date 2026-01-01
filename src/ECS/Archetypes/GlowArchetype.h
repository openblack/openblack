/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>

#include <entt/fwd.hpp>

#include "ECS/Components/Temple.h"

namespace openblack
{
struct LightEmitter;
}

namespace openblack::ecs::archetypes
{
class GlowArchetype
{
public:
	static std::array<entt::entity, 2> Create(const LightEmitter& emitter, openblack::ecs::components::TempleRoom room);
	GlowArchetype() = delete;
};
} // namespace openblack::ecs::archetypes
