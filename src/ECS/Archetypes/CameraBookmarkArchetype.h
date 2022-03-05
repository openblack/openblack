/*******************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 ******************************************************************************/

#pragma once

#include <array>

#include <entt/fwd.hpp>

namespace openblack::graphics
{
class Texture2D;
}

namespace openblack::ecs::archetypes
{

class CameraBookmarkArchetype
{
public:
	static std::array<entt::entity, 8> CreateAll(const graphics::Texture2D& texture);
};

} // namespace openblack::ecs::archetypes
