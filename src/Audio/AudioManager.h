/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <map>
#include <string>
#include <type_traits>
#include <vector>

#include <glm/glm.hpp>

#include "AudioDecoderInterface.h"
#include "AudioManagerInterface.h"
#include "AudioPlayer.h"
#include "Resources/Loaders.h"
#include "SoundGroup.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#warning "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack
{
class Game;
}

namespace openblack::audio
{

class AudioManager final: public AudioManagerInterface
{
public:
	AudioManager();
	~AudioManager();
	BufferId CreateBuffer(ChannelLayout layout, const std::vector<int16_t>& buffer, int sampleRate) override;
	void CreateBuffer(Sound& sound) override;
	void PlayEmitter(entt::entity emitter) override;
	void PauseEmitter(entt::entity emitter) override;
	void StopEmitter(entt::entity emitter) override;
	void DestroyEmitter(entt::entity emitter) override;
	entt::entity CreateEmitter(entt::id_type id, PlayType playType, glm::vec3 position, glm::vec3 direction, glm::vec2 radius,
	                           float volume, AudioStatus status, bool relative) override;
	[[nodiscard]] bool EmitterExists(entt::entity emitter) override;
	[[nodiscard]] float GetProgress(entt::entity entity) override;
	[[nodiscard]] AudioStatus GetStatus(entt::entity emitter) override;
	void PlayMusic(const std::string& packPath, PlayType type) override;
	void StopMusic() override;
	const Sound& GetSound(entt::id_type id) override;
	void PlaySound(entt::id_type id, PlayType type) override;
	void SetGlobalVolume(float volume) override { _globalVolume = volume; }
	void SetSfxVolume(float volume) override { _sfxVolume = volume; }
	void SetMusicVolume(float volume) override { _musicVolume = volume; }
	[[nodiscard]] float GetGlobalVolume() override { return _globalVolume; }
	[[nodiscard]] float GetSfxVolume() override { return _sfxVolume; }
	[[nodiscard]] float GetMusicVolume() override { return _musicVolume; }
	void Stop() override;
	void Update(Game& game) override;
	void CreateSoundGroup(const std::string& name) override;
	void AddMusicEntry(const std::string& name) override;
	[[nodiscard]] const std::vector<std::string>& GetMusicTracks() const override { return _music; }
	void AddToSoundGroup(const std::string& name, entt::id_type id) override;
	const SoundGroup& GetSoundGroup(const std::string& name) override;
	const std::map<std::string, SoundGroup>& GetSoundGroups() override;

private:
	std::unique_ptr<AudioPlayerInterface> _audioPlayer;
	/// All sounds are loaded
	std::map<std::string, SoundGroup> _soundGroups;
	/// Music resources are loaded on demand to avoid storing large audio buffers. There are no resource IDs yet
	std::vector<std::string> _music;
	float _globalVolume {1.0f};
	float _musicVolume {1.0f};
	float _sfxVolume {1.0f};
	entt::entity _musicEntity {entt::null};
};

} // namespace openblack::audio
