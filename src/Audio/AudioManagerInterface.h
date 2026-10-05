/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <entt/fwd.hpp>

#include "AnimEffectTable.h"
#include "AudioDecoderInterface.h"
#include "AudioPlayerInterface.h"
#include "ECS/Components/AudioEmitter.h"
#include "MusicPlayer.h"
#include "Sound.h"
#include "SoundGroup.h"

namespace openblack
{

namespace pack
{
struct AudioBankSampleHeader;
}

namespace audio
{

class AtmosPlayer;

/// A music bank's group and its number of samples
struct MusicBankInfo
{
	int32_t groupId;
	uint32_t chunkCount;
};

class AudioManagerInterface
{
public:
	virtual void Stop() = 0;
	virtual void Update() = 0;
	virtual BufferId CreateBuffer(ChannelLayout layout, const std::vector<int16_t>& buffer, int sampleRate) = 0;
	virtual void CreateBuffer(Sound& sound) = 0;
	virtual void PlayEmitter(entt::entity emitter) = 0;
	virtual void PauseEmitter(entt::entity emitter) = 0;
	virtual void StopEmitter(entt::entity emitter) = 0;
	virtual void DestroyEmitter(entt::entity emitter) = 0;
	/// An emitter for a loaded sound, ready to play. With a world position it is 3D audio sounding from there,
	/// otherwise 2D audio centred on the listener. Repeat loops it forever, otherwise the bank header's loop count
	/// applies.
	virtual entt::entity CreateEmitter(entt::id_type id, std::optional<glm::vec3> worldPosition, PlayType playType) = 0;
	virtual bool EmitterExists(entt::entity emitter) = 0;
	[[nodiscard]] virtual float GetProgress(entt::entity emitter) = 0;
	[[nodiscard]] virtual AudioStatus GetStatus(entt::entity emitter) = 0;
	virtual void SetGlobalVolume(float volume) = 0;
	virtual void SetSfxVolume(float volume) = 0;
	virtual void SetMusicVolume(float volume) = 0;
	[[nodiscard]] virtual float GetGlobalVolume() = 0;
	[[nodiscard]] virtual float GetSfxVolume() = 0;
	[[nodiscard]] virtual float GetMusicVolume() = 0;
	/// Plays a music bank on its own, from the start, stopping any other music at once
	virtual void PlayMusic(const std::string& packPath, PlayType type) = 0;
	virtual void StopMusic() = 0;
	/// Plays the music bank at a path, loading it if it is not playing already. options.bank is ignored.
	virtual bool MusicPlay(const std::string& bankPath, const MusicPlayOptions& options) = 0;
	/// Stops the music, fading it out if asked
	virtual void MusicStop(bool fadeOut) = 0;
	/// Whether music is playing
	[[nodiscard]] virtual bool MusicIsActive() const = 0;
	/// The music group and length of a bank, null if there is no such bank
	[[nodiscard]] virtual std::optional<MusicBankInfo> GetMusicBankInfo(const std::string& bankPath) = 0;
	/// Null when there is no audio device
	[[nodiscard]] virtual const MusicPlayer* GetMusic() const = 0;
	/// Plays a sound as 2D audio
	virtual void PlaySound(entt::id_type id, PlayType type) = 0;
	/// A one-shot sound effect played the way the game plays its sound effects: volume, pitch, loop and distances come
	/// from the bank header where it overrides them. With a world position it is a 3D sound anchored there, otherwise
	/// it is centred on the listener.
	virtual void PlaySoundEffect(entt::id_type id, std::optional<glm::vec3> worldPosition) = 0;
	/// Stops the sound effects of a sound that PlaySoundEffect started, as one that loops forever goes on until then
	virtual void StopSoundEffect(entt::id_type id) = 0;
	/// The animation effects of a loaded sound bank, named as its sounds are ("<bank>/<sample id>")
	virtual void AddAnimEffects(const std::string& bankName, AnimEffectTable table) = 0;
	/// One of the samples a bank's animation effects pick for keys, chosen at random, as
	/// a one-shot 3D sound at position on behalf of owner. Nothing plays when the listener is beyond the sample's
	/// maximum distance, or when the bank header plays the sample once and owner is playing it, or another of its
	/// voice group, already.
	virtual void PlayAnimEffect(const std::string& bankName, std::span<const int32_t> keys, entt::entity owner,
	                            const glm::vec3& position) = 0;
	virtual const Sound& GetSound(entt::id_type id) = 0;
	virtual void CreateSoundGroup(const std::string& name) = 0;
	virtual void AddToSoundGroup(const std::string& name, entt::id_type id) = 0;
	virtual const SoundGroup& GetSoundGroup(const std::string& name) = 0;
	virtual const std::map<std::string, SoundGroup>& GetSoundGroups() = 0;
	virtual void AddMusicEntry(const std::string& name) = 0;
	[[nodiscard]] virtual const std::vector<std::string>& GetMusicTracks() const = 0;

	// Atmosphere banks, as the game's audio library keeps them. Bank 0 is no bank and is ignored.
	virtual uint32_t AtmosRegisterBank(const std::string& bankName, const std::vector<pack::AudioBankSampleHeader>& headers,
	                                   uint16_t atmosCount) = 0;
	virtual void AtmosReleaseBank(uint32_t bank) = 0;
	virtual void AtmosSetBankVolume(uint32_t bank, int32_t volume) = 0;
	virtual void AtmosSetGroup(uint32_t bank, uint32_t group) = 0;
	virtual void AtmosProcess(bool active) = 0;
	/// Null when there is no audio device
	[[nodiscard]] virtual const AtmosPlayer* GetAtmos() const = 0;
};
} // namespace audio
} // namespace openblack
