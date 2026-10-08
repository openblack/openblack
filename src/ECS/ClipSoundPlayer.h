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

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include "3D/AllMeshes.h"

namespace openblack
{
class L3DAnim;
}

/// The sounds placed on a living thing's clip, played as the clip it is drawn with plays on
namespace openblack::ecs::clip_sound_player
{

/// Plays the sounds a living thing's clip passes as it plays on from a place by so many milliseconds, from where the
/// thing is: a person's sounds by its size and only while it is alive, the banter from the villagers' bank (the first
/// from its home), a thrown person's scream only early in its flight, anything else from the editor's bank. Nothing
/// for a clip played once that has already finished.
void Play(entt::entity entity, AnimId clipId, const L3DAnim& clip, uint32_t place, uint32_t played, const glm::vec3& position);

} // namespace openblack::ecs::clip_sound_player
