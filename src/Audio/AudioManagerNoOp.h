/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "AudioManagerInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp", use interface instead.
#endif

namespace openblack::audio
{

class AudioManagerNoOp final: public AudioManagerInterface
{
public:
	BufferId CreateBuffer([[maybe_unused]] ChannelLayout layout, [[maybe_unused]] const std::vector<int16_t>& buffer,
	                      [[maybe_unused]] int sampleRate) override
	{
		return 0;
	}
	void CreateBuffer([[maybe_unused]] Sound& sound) override {}
	void PlayEmitter([[maybe_unused]] entt::entity emitter) override {}
	void PauseEmitter([[maybe_unused]] entt::entity emitter) override {}
	void StopEmitter([[maybe_unused]] entt::entity emitter) override {}
	void DestroyEmitter([[maybe_unused]] entt::entity emitter) override {}
	entt::entity CreateEmitter([[maybe_unused]] entt::id_type id, [[maybe_unused]] PlayType playType,
	                           [[maybe_unused]] glm::vec3 position, [[maybe_unused]] glm::vec3 direction,
	                           [[maybe_unused]] glm::vec2 radius, [[maybe_unused]] float volume,
	                           [[maybe_unused]] AudioStatus status, [[maybe_unused]] bool relative) override
	{
		return {};
	};
	[[nodiscard]] bool EmitterExists([[maybe_unused]] entt::entity emitter) override { return false; }
	[[nodiscard]] float GetProgress([[maybe_unused]] entt::entity emitter) override { return 1.0f; }
	[[nodiscard]] AudioStatus GetStatus([[maybe_unused]] entt::entity emitter) override { return {}; }
	void PlayMusic([[maybe_unused]] const std::string& packPath, [[maybe_unused]] PlayType type) override {}
	void StopMusic() override {}
	const Sound& GetSound([[maybe_unused]] entt::id_type id) override
	{
		static const Sound result {};
		return result;
	}
	void PlaySound([[maybe_unused]] entt::id_type id, [[maybe_unused]] PlayType type) override {}
	void SetGlobalVolume([[maybe_unused]] float volume) override {}
	void SetSfxVolume([[maybe_unused]] float volume) override {}
	void SetMusicVolume([[maybe_unused]] float volume) override {}
	[[nodiscard]] float GetGlobalVolume() override { return 0.0f; }
	[[nodiscard]] float GetSfxVolume() override { return 0.0f; }
	[[nodiscard]] float GetMusicVolume() override { return 0.0f; }
	void Stop() override {}
	void Update() override {}
	void CreateSoundGroup([[maybe_unused]] const std::string& name) override {}
	void AddMusicEntry([[maybe_unused]] const std::string& name) override {}
	[[nodiscard]] const std::vector<std::string>& GetMusicTracks() const override
	{
		static const std::vector<std::string> result;
		return result;
	}
	void AddToSoundGroup([[maybe_unused]] const std::string& name, [[maybe_unused]] entt::id_type id) override {}
	const SoundGroup& GetSoundGroup([[maybe_unused]] const std::string& name) override
	{
		static const SoundGroup result;
		return result;
	}
	const std::map<std::string, SoundGroup>& GetSoundGroups() override
	{
		static const std::map<std::string, SoundGroup> result;
		return result;
	}
};

} // namespace openblack::audio
