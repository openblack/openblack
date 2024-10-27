/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <entt/fwd.hpp>
#include <glm/gtx/transform.hpp>

#include "Graphics/DebugLines.h"
#include "Graphics/Mesh.h"

using namespace openblack::graphics;

namespace openblack::ecs::components
{

struct DebugCross
{
	std::unique_ptr<Mesh> debugCross {DebugLines::CreateCross()};
	glm::mat4 debugCrossPose {glm::identity<glm::mat4>()};
};

} // namespace openblack::ecs::components
