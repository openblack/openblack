/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "Graphics/Mesh.h"
#include "Graphics/Primitive.h"

namespace openblack::ecs::components
{

struct SpriteMesh
{
	std::unique_ptr<openblack::graphics::Mesh> plane {openblack::graphics::Primitive::CreatePlane()};
};

} // namespace openblack::ecs::components
