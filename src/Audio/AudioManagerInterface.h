/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <entt/fwd.hpp>

#include "AudioDecoderInterface.h"
#include "AudioPlayerInterface.h"
#include "ECS/Components/AudioEmitter.h"
#include "Resources/Loaders.h"
#include "Sound.h"
#include "SoundGroup.h"

namespace openblack
{

class Game;

namespace audio
{
class AudioManagerInterface
{
public:
	virtual void Stop() = 0;
	virtual void Update(Game& game) = 0;
	virtual BufferId CreateBuffer(ChannelLayout layout, const std::vector<int16_t>& buffer, int sampleRate) = 0;
	virtual void CreateBuffer(Sound& sound) = 0;
	virtual void PlayEmitter(entt::entity emitter) = 0;
	virtual void PauseEmitter(entt::entity emitter) = 0;
	virtual void StopEmitter(entt::entity emitter) = 0;
	virtual void DestroyEmitter(entt::entity emitter) = 0;
	virtual entt::entity CreateEmitter(entt::id_type id, PlayType playType, glm::vec3 position, glm::vec3 direction,
	                                   glm::vec2 radius, float volume, AudioStatus status, bool relative) = 0;
	virtual bool EmitterExists(entt::entity emitter) = 0;
	[[nodiscard]] virtual float GetProgress(entt::entity emitter) = 0;
	[[nodiscard]] virtual AudioStatus GetStatus(entt::entity emitter) = 0;
	virtual void SetGlobalVolume(float volume) = 0;
	virtual void SetSfxVolume(float volume) = 0;
	virtual void SetMusicVolume(float volume) = 0;
	[[nodiscard]] virtual float GetGlobalVolume() = 0;
	[[nodiscard]] virtual float GetSfxVolume() = 0;
	[[nodiscard]] virtual float GetMusicVolume() = 0;
	virtual void PlayMusic(const std::string& packPath, PlayType type) = 0;
	virtual void StopMusic() = 0;
	virtual void PlaySound(entt::id_type id, PlayType type) = 0;
	virtual const Sound& GetSound(entt::id_type id) = 0;
	virtual void CreateSoundGroup(const std::string& name) = 0;
	virtual void AddToSoundGroup(const std::string& name, entt::id_type id) = 0;
	virtual const SoundGroup& GetSoundGroup(const std::string& name) = 0;
	virtual const std::map<std::string, SoundGroup>& GetSoundGroups() = 0;
	virtual void AddMusicEntry(const std::string& name) = 0;
	[[nodiscard]] virtual const std::vector<std::string>& GetMusicTracks() const = 0;
};
} // namespace audio
} // namespace openblack
