/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

extern "C" {
#include <AL/al.h>
#include <AL/alc.h>
}

#include <filesystem>
#include <queue>
#include <vector>

#include <glm/glm.hpp>

#include "Sound.h"

namespace openblack::audio
{

class AudioPlayerInterface
{
public:
	virtual ~AudioPlayerInterface() = default;
	virtual void Initialize() = 0;
	virtual void UpdateListener(glm::vec3 pos, glm::vec3 vel, glm::vec3 front, glm::vec3 up) const = 0;
	[[nodiscard]] virtual BufferId CreateBuffer(ChannelLayout layout, const std::vector<int16_t>& buffer, int sampleRate) = 0;
	virtual void QueueBuffer(SourceId sourceId, BufferId buffer) = 0;
	virtual void DeleteBuffer(BufferId id) = 0;
	[[nodiscard]] virtual SourceId CreateSource(float pitch, bool relative) = 0;
	virtual void DeleteSource(SourceId id) = 0;
	virtual void UpdateSource(SourceId id, glm::vec3 pos, float volume, bool loop) = 0;
	virtual void UpdateSource(SourceId id, float volume, bool loop) = 0;
	[[nodiscard]] virtual float GetDuration(BufferId id) = 0;
	virtual void PlaySource(SourceId id, glm::vec3 pos, float volume, bool loop) = 0;
	virtual void PlaySource(SourceId id, float volume, bool loop) = 0;
	virtual void PauseSource(SourceId id) const = 0;
	virtual void StopSource(SourceId id) const = 0;
	virtual void SetVolume(SourceId id, float volume) = 0;
	[[nodiscard]] virtual float GetVolume() const = 0;
	[[nodiscard]] virtual AudioStatus GetStatus(SourceId id) const = 0;
	[[nodiscard]] virtual float GetProgress(size_t sizeInBytes, SourceId sourceId) const = 0;
};
} // namespace openblack::audio
