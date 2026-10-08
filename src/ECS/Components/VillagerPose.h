/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <vector>

#include <glm/mat4x4.hpp>

#include "3D/AllMeshes.h"

namespace openblack::ecs::components
{

/// How a villager is drawn this frame: the clip its state plays, its place in it in milliseconds, and the bones posed
/// from it. With no bones the villager is drawn in the pose its model rests in.
struct VillagerPose
{
	AnimId clip {AnimId::Invalid};
	uint32_t place {0};
	std::vector<glm::mat4> bones;
};

} // namespace openblack::ecs::components
