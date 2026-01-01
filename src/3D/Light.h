/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <vector>

#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace openblack
{

struct Glow
{
	glm::vec4 backgroundColour;
	glm::vec4 brightSpotColour;
	float backgroundScale;
	float brightSpotScale;
	glm::vec3 position;
};

struct LightEmitter
{
	Glow glow;
};

struct Lights
{
	std::vector<LightEmitter> emitters;
};
} // namespace openblack
