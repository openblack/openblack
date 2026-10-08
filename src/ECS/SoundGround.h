/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>

#include <glm/vec3.hpp>

#include "Creature/CreatureAudio.h"

namespace openblack::ecs::sound_ground
{

/// The ground under a point as the sounds see it: water, or the sound surface of its terrain material (snow where it
/// lies deep enough); nothing off the map or where there is no land
[[nodiscard]] std::optional<creature_audio::Ground> At(const glm::vec3& position);

} // namespace openblack::ecs::sound_ground
