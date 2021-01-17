/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "AudioEmitter.h"
#include "SoundPack.h"

namespace openblack::audio
{

enum class AudioStatus
{
	Initial,
	Playing,
	Paused,
	Stopped
};

class AudioPlayer
{
public:
	virtual ~AudioPlayer() = default;
	virtual void Activate() = 0;
	// Global audio control
	virtual void CleanUpResources(std::map<AudioEmitterId, AudioEmitter>& emitters) = 0;
	[[nodiscard]] virtual std::string GetName() const = 0;
	[[nodiscard]] virtual float GetVolume() const = 0;
	virtual void SetVolume(float volume) = 0;
	virtual void UpdateListenerState(glm::vec3 pos, glm::vec3 vel, glm::vec3 front, glm::vec3 up) const = 0;

	// Emitter sound control. Used for 3D sounds
	virtual void SetupEmitter(AudioEmitter& emitter, Sound& sound) = 0;
	virtual void PlayEmitter(AudioEmitter& emitter) const = 0;
	virtual void PauseEmitter(AudioEmitter& emitter) const = 0;
	virtual void StopEmitter(AudioEmitter& emitter) const = 0;
	virtual void UpdateEmitterState(AudioEmitter& emitter) const = 0;
	virtual void CleanUpResource(AudioEmitter& emitter) const = 0;
	[[nodiscard]] virtual float GetAudioProgress(AudioEmitter& emitter) const = 0;

	// Lower level sound control. Used for music and GUI
	virtual void SetVolume(AudioSourceId id, float volume) = 0;
	[[nodiscard]] virtual float GetVolume(AudioSourceId id) const = 0;
	[[nodiscard]] virtual AudioStatus GetAudioStatus(AudioSourceId id) const = 0;
};

class MockAudioPlayer final : public AudioPlayer
{
public:
	MockAudioPlayer() { _volume = 0; }
	void Activate() override {}
	void CleanUpResources(std::map<AudioEmitterId, AudioEmitter>& emitters) override {}
	std::string GetName() const override { return "Mock Audio Player"; }
	void SetVolume(float volume) override { _volume = volume; }
	void UpdateListenerState(glm::vec3, glm::vec3, glm::vec3, glm::vec3) const override {}
	float GetVolume() const override { return _volume; }
	void SetVolume(AudioSourceId id, float volume) override {}
	float GetVolume(AudioSourceId id) const override { return _volume; }
	void SetupEmitter(AudioEmitter&, Sound&) override {}
	void PlayEmitter(AudioEmitter& emitter) const override {}
	void PauseEmitter(AudioEmitter& emitter) const override {}
	void StopEmitter(AudioEmitter& emitter) const override {}
	void UpdateEmitterState(AudioEmitter& emitter) const override {}
	void CleanUpResource(AudioEmitter& emitter) const override {};
	AudioStatus GetAudioStatus(AudioSourceId id) const override { return AudioStatus::Paused; }
	float GetAudioProgress(AudioEmitter&) const override { return 0; }

private:
	float _volume;
};
} // namespace openblack::audio
