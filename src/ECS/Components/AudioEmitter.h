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
	/// The volume the gain was set from, 0 to 127
	uint32_t volume = 127;
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
	/// What the sound is played for, such as the tree a rustle comes from. A sound whose bank header plays it once
	/// does not start again for the same owner while it, or another of its voice group, plays.
	entt::entity owner {entt::null};
	/// The bank the sound is from and its voice group there (Sound::group)
	entt::id_type bank {0};
	uint16_t group {0};
};
} // namespace openblack::ecs::components
