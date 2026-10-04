/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "AtmosPlayer.h"
#include "AudioDecoderInterface.h"
#include "AudioManagerInterface.h"
#include "AudioPlayer.h"
#include "MusicPlayer.h"
#include "MusicStreamBackend.h"
#include "SoundGroup.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack
{
class Game;
}

namespace openblack::audio
{

class AudioManager final: public AudioManagerInterface, private VoiceBackend
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
	entt::entity CreateEmitter(entt::id_type id, std::optional<glm::vec3> worldPosition, PlayType playType) override;
	[[nodiscard]] bool EmitterExists(entt::entity emitter) override;
	[[nodiscard]] float GetProgress(entt::entity entity) override;
	[[nodiscard]] AudioStatus GetStatus(entt::entity emitter) override;
	void PlayMusic(const std::string& packPath, PlayType type) override;
	void StopMusic() override;
	bool MusicPlay(const std::string& bankPath, const MusicPlayOptions& options) override;
	void MusicStop(bool fadeOut) override;
	[[nodiscard]] bool MusicIsActive() const override;
	[[nodiscard]] std::optional<MusicBankInfo> GetMusicBankInfo(const std::string& bankPath) override;
	[[nodiscard]] const MusicPlayer* GetMusic() const override { return _musicPlayer.get(); }
	const Sound& GetSound(entt::id_type id) override;
	void PlaySound(entt::id_type id, PlayType type) override;
	void PlaySoundEffect(entt::id_type id, std::optional<glm::vec3> worldPosition) override;
	void SetGlobalVolume(float volume) override { _globalVolume = volume; }
	void SetSfxVolume(float volume) override { _sfxVolume = volume; }
	void SetMusicVolume(float volume) override { _musicVolume = volume; }
	[[nodiscard]] float GetGlobalVolume() override { return _globalVolume; }
	[[nodiscard]] float GetSfxVolume() override { return _sfxVolume; }
	[[nodiscard]] float GetMusicVolume() override { return _musicVolume; }
	void Stop() override;
	void Update() override;
	void CreateSoundGroup(const std::string& name) override;
	void AddMusicEntry(const std::string& name) override;
	[[nodiscard]] const std::vector<std::string>& GetMusicTracks() const override { return _music; }
	void AddToSoundGroup(const std::string& name, entt::id_type id) override;
	const SoundGroup& GetSoundGroup(const std::string& name) override;
	const std::map<std::string, SoundGroup>& GetSoundGroups() override;
	uint32_t AtmosRegisterBank(const std::string& bankName, const std::vector<pack::AudioBankSampleHeader>& headers,
	                           uint16_t atmosCount) override;
	void AtmosReleaseBank(uint32_t bank) override;
	void AtmosSetBankVolume(uint32_t bank, int32_t volume) override;
	void AtmosSetGroup(uint32_t bank, uint32_t group) override;
	void AtmosProcess(bool active) override;
	[[nodiscard]] const AtmosPlayer* GetAtmos() const override { return _atmos.get(); }

private:
	// VoiceBackend
	[[nodiscard]] Handle Start(const VoiceStart& start) override;
	[[nodiscard]] bool IsPlaying(Handle handle) const override;
	void SetVolume(Handle handle, uint32_t volume) override;
	void Stop(Handle handle) override;

	struct AtmosVoice
	{
		SourceId source;
		float gain;
	};

	/// A source for a voice of a loaded sound, decoding the sound first if needed
	SourceId CreateSource(Sound& sound, const VoiceStart& start);
	/// Places a source in LHAudio's listener frame, or centres it
	void PositionSource(SourceId source, std::optional<glm::vec3> listenerPosition, float distanceScale);
	/// LHSamplePlay's play parameters for a sample, with the bank header overriding LH_SamplePlayOptions' defaults
	[[nodiscard]] static VoiceStart MakeVoiceStart(const Sound& sound, std::optional<glm::vec3> worldPosition,
	                                               PlayType playType);
	/// A music bank loaded from a path relative to the game, shared with any channel playing it already
	[[nodiscard]] std::shared_ptr<const MusicBank> LoadMusicBank(const std::string& bankPath);
	/// LHAudio's listener frame (x right, y forward, z up) for a world position
	[[nodiscard]] static glm::vec3 ToListenerFrame(glm::vec3 worldPosition);

	std::unique_ptr<AudioPlayerInterface> _audioPlayer;
	/// All sounds are loaded
	std::map<std::string, SoundGroup> _soundGroups;
	/// Music resources are loaded on demand to avoid storing large audio buffers. There are no resource IDs yet
	std::vector<std::string> _music;
	float _globalVolume {1.0f};
	float _musicVolume {1.0f};
	float _sfxVolume {1.0f};
	std::unique_ptr<MusicStreamBackend> _musicStreams;
	std::unique_ptr<MusicPlayer> _musicPlayer;
	/// Banks stay loaded while a channel plays them
	std::map<std::string, std::weak_ptr<const MusicBank>> _musicBanks;
	std::map<std::string, std::optional<MusicBankInfo>> _musicBankInfo;
	std::shared_ptr<const MusicBank> _recentMusicBank;
	std::chrono::steady_clock::time_point _lastMusicUpdate {std::chrono::steady_clock::now()};
	std::map<Handle, AtmosVoice> _atmosVoices;
	/// Decoded samples, kept for the lifetime of the audio manager
	std::vector<BufferId> _decodedBuffers;
	Handle _nextAtmosVoice {1};
	std::unique_ptr<AtmosPlayer> _atmos;
};

} // namespace openblack::audio
