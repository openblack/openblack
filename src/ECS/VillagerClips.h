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

#include <optional>

#include <glm/vec3.hpp>

#include "3D/AllMeshes.h"

namespace openblack::ecs::components
{
struct LivingAction;
}

/// What the villagers' states share about their clips and the ground under them
namespace openblack::ecs::villager_clips
{

/// A clip's length in milliseconds, none for one not loaded
[[nodiscard]] std::optional<float> ClipMilliseconds(AnimId clip);
/// Whether a clip has played through so many times since the villager went into its state
[[nodiscard]] bool ClipPlayed(const components::LivingAction& action, AnimId clip, uint32_t times = 1);
/// Whether the land under a point is water, the shallow shore included
[[nodiscard]] bool IsOnWater(glm::vec3 point);

} // namespace openblack::ecs::villager_clips
