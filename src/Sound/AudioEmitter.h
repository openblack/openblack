/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Sound.h"
#include <glm/glm.hpp>
#include <string>

namespace openblack::audio
{

using AudioEmitterId = size_t;
using AudioSourceId = unsigned int;
using AudioBufferId = unsigned int;

// Unused ID value. Use for lazy initialisation
const static AudioEmitterId InitialAudioEmitterId = -1;

struct AudioEmitter
{
	AudioSourceId audioSourceId;
	AudioBufferId audioBufferId;
	SoundId soundId;
	int priority = 0;
	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 direction;
	glm::vec2 radius;
	float volume = 0;
	PlayType loop = PlayType::None;
	bool world = false;
};
} // namespace openblack::audio
