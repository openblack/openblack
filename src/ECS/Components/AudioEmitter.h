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

#include <entt/fwd.hpp>
#include <glm/vec3.hpp>

#include "Audio/Sound.h"

namespace openblack::ecs::components
{

/// A sound playing the way LHSamplePlay plays a bank sample: its volume, pitch, loop and distances come from the
/// bank header where it overrides them. A 2D emitter sounds centred on the listener, a 3D one from a world position
/// with QSound's distance attenuation.
struct AudioEmitter
{
	audio::SourceId sourceId;
	entt::id_type soundId;
	int priority = 0;
	/// 3D audio, positioned in the world. 2D audio has no position.
	bool spatial = false;
	/// Where a 3D emitter sounds from, in world coordinates
	glm::vec3 position {0.0f, 0.0f, 0.0f};
	/// 0 to 1, before the global and the effect or music volume
	float gain = 1.0f;
	/// Playback rate in percent, after the random deviation
	uint32_t pitchPercent = 100;
	/// QSound distance mapping: full volume up to minDistance, then attenuated by minDistance / distance up to
	/// maxDistance, with distances multiplied by distanceScale
	float minDistance = 1.0f;
	float maxDistance = 9999.0f;
	float distanceScale = 0.3f;
	audio::PlayType loop = audio::PlayType::Once;
	audio::AudioStatus state = audio::AudioStatus::Initial;
	/// Follows the music volume rather than the effect volume
	bool music = false;
};
} // namespace openblack::ecs::components
