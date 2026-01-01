/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <entt/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Audio/Sound.h"

namespace openblack::ecs::components
{
struct AudioEmitter
{
	audio::SourceId sourceId;
	entt::id_type soundId;
	int priority = 0;
	glm::vec3 position;
	glm::vec3 direction;
	glm::vec2 radius;
	float volume = 0;
	audio::PlayType loop = audio::PlayType::Once;
	audio::AudioStatus state = audio::AudioStatus::Playing;
	bool relative;
};
} // namespace openblack::ecs::components
