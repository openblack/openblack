/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "AudioDecoder.h"
#include "FfmpegDecoder.h"
#include "OpenAlPlayer.h"
#include "SoundPack.h"
#include <SadFile.h>
#include <glm/glm.hpp>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

namespace openblack
{
class Game;
}

namespace openblack::audio
{

class SoundHandler
{
public:
	SoundHandler(std::unique_ptr<AudioDecoder>&& decoder, std::unique_ptr<AudioPlayer>&& player);
	void RegisterSoundPack(std::unique_ptr<SoundPack>& soundPack);
	void PlayEmitter(AudioEmitterId id);
	void PauseEmitter(AudioEmitterId id);
	void StopEmitter(AudioEmitterId id);
	void PlaySound(SoundId id);
	AudioEmitterId CreateEmitters(const std::vector<SoundId> ids, glm::vec3& position, glm::vec3& velocity,
	                              glm::vec3& direction, glm::vec2& radius, float volume, bool world, PlayType playType);
	AudioEmitterId CreateEmitter(SoundId id, glm::vec3& position, glm::vec3& velocity, glm::vec3& direction, glm::vec2& radius,
	                             float volume, bool world, PlayType playType);
	void DestroyEmitters();
	void DestroyEmitter(AudioEmitterId id);
	[[nodiscard]] bool EmitterExists(AudioEmitterId id);
	[[nodiscard]] const std::vector<std::shared_ptr<SoundPack>>& GetSoundPacks() const { return _soundPacks; }
	[[nodiscard]] std::unique_ptr<Sound>& GetSound(SoundId id);
	[[nodiscard]] float GetGlobalVolume() const { return _audioPlayer->GetVolume(); };
	[[nodiscard]] std::string GetAudioDecoderName() const { return _audioDecoder->GetName(); }
	[[nodiscard]] std::string GetAudioPlayerName() const { return _audioPlayer->GetName(); }
	[[nodiscard]] const std::map<AudioEmitterId, AudioEmitter>& GetEmitters() const;
	void ReplaceAudioLoader(std::unique_ptr<AudioDecoder> audioLoader) { _audioDecoder.reset(audioLoader.release()); }
	void ReplaceAudioPlayer(std::unique_ptr<AudioPlayer> audioPlayer);
	const std::unique_ptr<AudioPlayer>& GetPlayer() const { return _audioPlayer; }
	void SetGlobalVolume(float volume);
	void Tick(Game& game);

private:
	size_t _emitterId;
	std::unique_ptr<AudioPlayer> _audioPlayer;
	std::unique_ptr<AudioDecoder> _audioDecoder;
	std::map<SoundId, std::shared_ptr<SoundPack>> _soundIdLookup;
	std::vector<std::shared_ptr<SoundPack>> _soundPacks;
	std::map<AudioEmitterId, AudioEmitter> _emitters;
};

std::unique_ptr<SoundHandler> CreateSoundHandler() noexcept;
} // namespace openblack::audio