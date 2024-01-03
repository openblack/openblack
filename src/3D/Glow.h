/*******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <string>

#include <glm/vec3.hpp>

#include "ECS/Components/Transform.h"

namespace openblack
{

struct Glow
{
	std::string name;
	glm::vec4 colour;
	glm::vec3 scale;
};

struct GlowEmitter
{
	Glow glow;
	ecs::components::Transform transform;
};

struct Glows
{
	std::vector<GlowEmitter> entries;
};
} // namespace openblack
