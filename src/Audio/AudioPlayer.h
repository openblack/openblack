/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "AudioPlayerInterface.h"

namespace openblack::audio
{

class AudioPlayer final: public AudioPlayerInterface
{
public:
	AudioPlayer();
	~AudioPlayer() override;
	void Initialize() override;
	void UpdateListener(glm::vec3 pos, glm::vec3 vel, glm::vec3 front, glm::vec3 up) const override;
	BufferId CreateBuffer(ChannelLayout layout, const std::vector<int16_t>& buffer, int sampleRate) override;
	void QueueBuffer(SourceId sourceId, BufferId buffer) override;
	void DeleteBuffer(BufferId id) override;
	void DeleteSource(SourceId id) override;
	void UpdateSource(SourceId id, glm::vec3 pos, float volume, bool loop) override;
	void UpdateSource(SourceId id, float volume, bool loop) override;
	float GetDuration(BufferId id) override;
	SourceId CreateSource(float pitch, bool relative) override;
	void PlaySource(SourceId id, glm::vec3 pos, float volume, bool loop) override;
	void PlaySource(SourceId id, float volume, bool loop) override;
	void PauseSource(SourceId id) const override;
	void StopSource(SourceId id) const override;
	void SetVolume(SourceId id, float volume) override;
	[[nodiscard]] float GetVolume() const override;
	[[nodiscard]] AudioStatus GetStatus(SourceId id) const override;
	[[nodiscard]] float GetProgress(size_t sizeInBytes, SourceId sourceId) const override;

private:
	static void SetupLogging();
	static void DeleteDevice(ALCdevice* device);
	static void DeleteContext(ALCcontext* context);
	std::unique_ptr<ALCdevice, decltype(&DeleteDevice)> _device;
	std::unique_ptr<ALCcontext, decltype(&DeleteContext)> _context;
	float _volume {1.0f};
};
} // namespace openblack::audio
