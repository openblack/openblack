/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

extern "C" {
#include <AL/al.h>
#include <AL/alc.h>
}

#include <filesystem>
#include <queue>
#include <vector>

#include <glm/vec3.hpp>

#include "Sound.h"

namespace openblack::audio
{

class AudioPlayerInterface
{
public:
	virtual ~AudioPlayerInterface() = default;
	virtual void Initialize() = 0;
	/// Where the listener is and which way it faces. It has no velocity: the game's sounds don't shift with the camera's
	/// movement, which would silence them whenever it outran the speed of sound.
	virtual void UpdateListener(glm::vec3 pos, glm::vec3 front, glm::vec3 up) const = 0;
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
	virtual void SetPitch(SourceId id, float pitch) = 0;
	/// Position in OpenAL coordinates, without any axis swizzling
	virtual void SetPosition(SourceId id, glm::vec3 position) = 0;
	virtual void SetDistanceAttenuation(SourceId id, float referenceDistance, float maxDistance, float rolloff) = 0;
	virtual void SetLooping(SourceId id, bool loop) = 0;
	/// Start playback without touching any source parameters
	virtual void StartSource(SourceId id) = 0;
	[[nodiscard]] virtual float GetVolume() const = 0;
	[[nodiscard]] virtual AudioStatus GetStatus(SourceId id) const = 0;
	[[nodiscard]] virtual float GetProgress(size_t sizeInBytes, SourceId sourceId) const = 0;
	/// Takes the buffers a streaming source has finished playing off its queue, oldest first
	[[nodiscard]] virtual std::vector<BufferId> UnqueueProcessedBuffers(SourceId id) = 0;
	/// Frames played into the oldest buffer queued on a source
	[[nodiscard]] virtual uint32_t GetSampleOffset(SourceId id) const = 0;
};
} // namespace openblack::audio
