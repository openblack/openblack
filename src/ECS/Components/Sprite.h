/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "Graphics/GraphicsHandle.h"

namespace openblack::ecs::components
{
struct Sprite
{
	graphics::TextureHandle texture;
	glm::vec2 uvMin;
	glm::vec2 uvExtent;
	glm::vec4 tint;
};

} // namespace openblack::ecs::components
