/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <entt/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

using namespace openblack::audio;

namespace openblack::ecs::components
{
struct AudioEmitter
{
	SourceId sourceId;
	entt::id_type soundId;
	int priority = 0;
	glm::vec3 position;
	glm::vec3 direction;
	glm::vec2 radius;
	float volume = 0;
	PlayType loop = PlayType::Once;
	AudioStatus state = AudioStatus::Playing;
	bool relative;
};
} // namespace openblack::ecs::components
